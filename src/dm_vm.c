#define __STDC_WANT_LIB_EXT2__ 1
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>

#include <dm_vm.h>
#include <dm_compiler.h>
#include <dm_chunk.h>
#include <dm.h>

typedef dm_value (dm_value_binary_fn)(dm_state*, dm_value, dm_value);
typedef int (dm_value_cmp_fn)(dm_state*, dm_value, dm_value);

typedef struct {
	int size;
	int capacity;
	dm_value *data;
} dm_stack;

static void stack_init(dm_stack *stack) {
	stack->size = 0;
	stack->capacity = 64;
	stack->data = malloc(sizeof(dm_value) * stack->capacity);
}

static void stack_free(dm_stack *stack) {
	free(stack->data);
	stack->size = 0;
	stack->capacity = 0;
	stack->data = NULL;
}

static void stack_push(dm_stack *stack, dm_value val) {
	if (stack->size >= stack->capacity) {
		stack->capacity *= 2;
		stack->data = realloc(stack->data, stack->capacity * sizeof(dm_value));
	}
	stack->data[stack->size++] = val;
}

static dm_value stack_pop(dm_stack *stack) {
	if (stack->size == 0) {
		//dm_runtime_error(dm, "can't pop from empty stack");
		return dm_value_nil();
	}

	stack->size--;
	return stack->data[stack->size];
}

static dm_value stack_peek(dm_stack *stack) {
	if (stack->size == 0) {
		//dm_runtime_error(dm, "can't peek empty stack");
		return dm_value_nil();
	}

	return stack->data[stack->size - 1];
}

static dm_value stack_peekn(dm_stack *stack, int n) {
	if (stack->size <= n) {
		//dm_runtime_error(dm, "can't peek %d items in stack of size %d", n, stack->size);
		return dm_value_nil();
	}

	return stack->data[stack->size - 1 - n];
}

static uint8_t read8(dm_chunk *chunk) {
	return chunk->code[chunk->ip++];
}

static uint16_t read16(dm_chunk *chunk) {
	uint16_t s = chunk->code[chunk->ip++];
	s <<= 8;
	s |= chunk->code[chunk->ip++];
	return s;
}

static bool is_falsey(dm_value val) {
	return val.type == DM_TYPE_NIL || (val.type == DM_TYPE_BOOL && val.bool_val == false);
}

dm_exception void dm_runtime_error(dm_state *dm, const char *message, ...) {
	va_list args;
	va_start(args, message);
	dm_state_set_error(dm, message, args);
}

dm_exception void dm_runtime_compare_mismatch(dm_state *dm, dm_value v1, dm_value v2) {
	const char *msg = "Can't compare <%s> and <%s>";
	const char *ty1 = dm_value_type_str(dm, v1);
	const char *ty2 = dm_value_type_str(dm, v2);
	dm_runtime_error(dm, msg, ty1, ty2);
}

dm_exception void dm_runtime_no_method_error(dm_state *dm, const char *method, dm_value v) {
	const char *ty = dm_value_type_str(dm, v);
	dm_runtime_error(dm, "Unknown method '%s' for <%s>", method, ty);
}

dm_exception void dm_runtime_type_mismatch(dm_state *dm, dm_type expected, dm_value got) {
	const char *ty_exp = dm_state_get_module(dm, expected)->typename;
	const char *ty_got = dm_value_type_str(dm, got);
	dm_runtime_error(dm, "Expected value of type <%s>, got <%s>", ty_exp, ty_got);
}

dm_exception void dm_runtime_type_mismatch2(dm_state *dm, dm_type expected1, dm_type expected2, dm_value got) {
	const char *ty_exp1 = dm_state_get_module(dm, expected1)->typename;
	const char *ty_exp2 = dm_state_get_module(dm, expected2)->typename;
	const char *ty_got = dm_value_type_str(dm, got);
	dm_runtime_error(dm, "Expected value of type <%s> or <%s>, got <%s>", ty_exp1, ty_exp2, ty_got);
}

static dm_value print_backtrace(dm_state *dm, dm_value f) {
	dm_chunk *chunk = f.func_val->chunk;
	printf("    in ");
	dm_value_inspect(dm, f);
	printf("(%d)\n", dm_chunk_current_line(chunk));
	return dm_value_nil();
}

static dm_value do_import(dm_state *dm, dm_value module) {
	// TODO: search for builtin modules
	char *cwd = getcwd(NULL, 0);
	if (cwd == NULL) {
		return dm_value_nil();
	}

	char *file;
	asprintf(&file, "%s/%s.dm", cwd, dm_string_c_str(module.str_val));
	char *prog = dm_read_file(file);

	free(file);
	free(cwd);
	dm_value ret;
	if (dm_vm_exec(dm, prog, &ret, false) != 0) {
		return dm_value_nil();
	}

	return ret;
}

static dm_value do_opassign(dm_state *dm, dm_opassign op, dm_value old, dm_value v) {
	dm_module *m = dm_state_get_module(dm, old.type);
	switch (op) {
		case DM_OPASSIGN_PLUS:  return m->add(dm, old, v);
		case DM_OPASSIGN_MINUS: return m->sub(dm, old, v);
		case DM_OPASSIGN_MUL:   return m->mul(dm, old, v);
		case DM_OPASSIGN_DIV:   return m->div(dm, old, v);
		case DM_OPASSIGN_MOD:   return m->mod(dm, old, v);
	}

	dm_runtime_error(dm, "Can't execute op-assign %d", op);
}

static dm_value exec_func(dm_state *dm, dm_value f, dm_stack *stack) {
	dm_chunk *chunk = (dm_chunk*) f.func_val->chunk;
	chunk->ip = 0;

	dm_value self = dm_value_nil();
	if (f.func_val->takes_self && f.func_val->nargs > 0) {
		self = dm_chunk_get_var(chunk, 0);
	}

	if (setjmp(*dm_state_get_jmpbuf(dm)) != 0) {
		return dm_value_nil();
	}

	for (;;) {
		dm_opcode opcode = (dm_opcode) read8(chunk);

		switch (opcode) {
			case DM_OP_IMPORT:              {
				dm_value module = stack_pop(stack);
				if (module.type != DM_TYPE_STRING) {
					dm_runtime_error(dm, "Expected string for import");
				}

				dm_value v = do_import(dm, module);
				stack_push(stack, v);
				break;
			}
			case DM_OP_VARSET:              {
				int index = read16(chunk);
				dm_value v = stack_peek(stack);
				dm_chunk_set_var(chunk, index, v);
				break;
			}
			case DM_OP_VARGETOPSET:         {
				int opassign = read8(chunk);
				int index = read16(chunk);
				dm_value old = dm_chunk_get_var(chunk, index);
				dm_value v = stack_pop(stack);
				v = do_opassign(dm, opassign, old, v);
				stack_push(stack, v);
				dm_chunk_set_var(chunk, index, v);
				break;
			}
			case DM_OP_VARSET_UP:           {
				int ups = read8(chunk);
				int index = read16(chunk);
				dm_chunk *upchunk = chunk;
				for (int i = 0; i < ups; i++) {
					upchunk = (dm_chunk*) upchunk->parent;
					if (upchunk == NULL) {
						dm_runtime_error(dm, "Can't set upvalue from up chunk %d", ups);
					}
				}
				dm_value v = stack_peek(stack);
				dm_chunk_set_var(upchunk, index, v);
				break;
			}
			case DM_OP_VARGETOPSET_UP:      {
				int opassign = read8(chunk);
				int ups = read8(chunk);
				int index = read16(chunk);
				dm_chunk *upchunk = chunk;
				for (int i = 0; i < ups; i++) {
					upchunk = (dm_chunk*) upchunk->parent;
					if (upchunk == NULL) {
						dm_runtime_error(dm, "Can't set upvalue from up chunk %d", ups);
					}
				}
				dm_value old = dm_chunk_get_var(upchunk, index);
				dm_value v = stack_pop(stack);
				v = do_opassign(dm, opassign, old, v);
				stack_push(stack, v);
				dm_chunk_set_var(upchunk, index, v);
				break;
			}
			case DM_OP_VARGET:              {
				int index = read16(chunk);
				dm_value v = dm_chunk_get_var(chunk, index);
				stack_push(stack, v);
				break;
			}
			case DM_OP_VARGET_UP:           {
				int ups = read8(chunk);
				int index = read16(chunk);
				dm_chunk *upchunk = chunk;
				for (int i = 0; i < ups; i++) {
					upchunk = (dm_chunk*) upchunk->parent;
					if (upchunk == NULL) {
						dm_runtime_error(dm, "Can't get upvalue from up chunk %d", ups);
					}
				}
				dm_value v = dm_chunk_get_var(upchunk, index);
				stack_push(stack, v);
				break;
			}
			case DM_OP_FIELDSET:            {
				dm_value v = stack_pop(stack);
				dm_value field = stack_pop(stack);
				dm_value table = stack_pop(stack);
				if (table.type == DM_TYPE_ARRAY) {
					dm_value_array_set(dm, table, field, v);
				} else if (table.type == DM_TYPE_TABLE) {
					dm_value_table_set(dm, table, field, v);
				} else {
					dm_runtime_type_mismatch2(dm, DM_TYPE_ARRAY, DM_TYPE_TABLE, field);
				}
				stack_push(stack, v);
				break;
			}
			case DM_OP_FIELDGETOPSET:       {
				int opassign = read8(chunk);
				dm_value v = stack_pop(stack);
				dm_value field = stack_pop(stack);
				dm_value table = stack_pop(stack);
				if (table.type == DM_TYPE_ARRAY) {
					dm_value old = dm_value_array_get(dm, table, field);
					v = do_opassign(dm, opassign, old, v);
					dm_value_array_set(dm, table, field, v);
				} else if (table.type == DM_TYPE_TABLE) {
					dm_value old = dm_value_table_get(dm, table, field);
					v = do_opassign(dm, opassign, old, v);
					dm_value_table_set(dm, table, field, v);
				} else {
					dm_runtime_type_mismatch2(dm, DM_TYPE_ARRAY, DM_TYPE_TABLE, field);
				}
				stack_push(stack, v);
				break;
			}
			case DM_OP_FIELDSET_S:			{
				dm_value v = stack_pop(stack);
				dm_value field = stack_pop(stack);
				dm_value table = stack_pop(stack);
				dm_module *m = dm_state_get_module(dm, table.type);
				const char *field_s = dm_string_c_str(field.str_val);
				if (!m->fieldset_s(dm, table, field_s, v)) {
					const char *ty = dm_value_type_str(dm, table);
					dm_runtime_error(dm, "Can't set field '%s' of <%s>", field_s, ty);
				}
				stack_push(stack, v);
				break;
			}
			case DM_OP_FIELDGETOPSET_S:       {
				int opassign = read8(chunk);
				dm_value old;
				dm_value v = stack_pop(stack);
				dm_value field = stack_pop(stack);
				dm_value table = stack_pop(stack);
				dm_module *m = dm_state_get_module(dm, table.type);
				const char *field_s = dm_string_c_str(field.str_val);
				if (!m->fieldget_s(dm, table, field_s, &old)) {
					const char *ty = dm_value_type_str(dm, table);
					dm_runtime_error(dm, "Can't get field '%s' of <%s>", field_s, ty);
				}
				v = do_opassign(dm, opassign, old, v);
				if (!m->fieldset_s(dm, table, field_s, v)) {
					const char *ty = dm_value_type_str(dm, table);
					dm_runtime_error(dm, "Can't set field '%s' of <%s>", field_s, ty);
				}
				stack_push(stack, v);
				break;
			}
			case DM_OP_FIELDGET:            {
				dm_value field = stack_pop(stack);
				dm_value table = stack_pop(stack);
				dm_value v;
				if (table.type == DM_TYPE_ARRAY) {
					v = dm_value_array_get(dm, table, field);
				} else if (table.type == DM_TYPE_TABLE) {
					v = dm_value_table_get(dm, table, field);
				} else {
					const char *msg = "Can't get field of <%s>, expected <array> or <table>";
					dm_runtime_error(dm, msg, dm_value_type_str(dm, table));
				}
				stack_push(stack, v);
				break;
			}
			case DM_OP_FIELDGET_S:			{
				dm_value field = stack_pop(stack);
				dm_value table = stack_pop(stack);
				dm_value v;
				dm_module *m = dm_state_get_module(dm, table.type);
				const char *field_s = dm_string_c_str(field.str_val);
				if (!m->fieldget_s(dm, table, field_s, &v)) {
					const char *ty = dm_value_type_str(dm, table);
					dm_runtime_error(dm, "Can't get field '%s' of <%s>", field_s, ty);
				}
				stack_push(stack, v);
				break;
			}
			case DM_OP_FIELDGET_PUSHPARENT: {
				dm_value field = stack_pop(stack);
				dm_value table = stack_peek(stack);
				dm_value v;
				if (table.type == DM_TYPE_ARRAY) {
					v = dm_value_array_get(dm, table, field);
				} else if (table.type == DM_TYPE_TABLE) {
					v = dm_value_table_get(dm, table, field);
				} else {
					const char *msg = "Can't get field of <%s>, expected <array> or <table>";
					dm_runtime_error(dm, msg, dm_value_type_str(dm, table));
				}
				stack_push(stack, v);
				break;
			}
			case DM_OP_FIELDGET_S_PUSHPARENT: {
				dm_value field = stack_pop(stack);
				dm_value table = stack_peek(stack);
				dm_value v;
				dm_module *m = dm_state_get_module(dm, table.type);
				const char *field_s = dm_string_c_str(field.str_val);
				if (!m->fieldget_s(dm, table, field_s, &v)) {
					const char *ty = dm_value_type_str(dm, table);
					dm_runtime_error(dm, "Can't get field '%s' of <%s>", field_s, ty);
				}
				stack_push(stack, v);
				break;
			}
			case DM_OP_CONSTANT:            {
				uint16_t index = read16(chunk);
				stack_push(stack, chunk->consts[index]);
				break;
			}
			case DM_OP_CONSTANT_SMALLINT:	{
				uint16_t val = read16(chunk);
				stack_push(stack, dm_value_int(val));
				break;
			}
			case DM_OP_ARRAYLIT:            {
				int elements = read16(chunk);
				dm_value arr = dm_value_array(dm, elements);
				while (elements--) {
					dm_value_array_set(dm, arr, dm_value_int(elements), stack_pop(stack));
				}
				stack_push(stack, arr);
				break;
			}
			case DM_OP_TABLELIT:            {
				int elements = read16(chunk);
				dm_value tab = dm_value_table(dm, elements);
				while (elements--) {
					dm_value value = stack_pop(stack);
					dm_value key = stack_pop(stack);
					dm_value_table_set(dm, tab, key, value);
				}
				stack_push(stack, tab);
				break;
			}
			case DM_OP_TRUE:                {
				stack_push(stack, dm_value_bool(true));
				break;
			}
			case DM_OP_FALSE:               {
				stack_push(stack, dm_value_bool(false));
				break;
			}
			case DM_OP_NIL:                 {
				stack_push(stack, dm_value_nil());
				break;
			}
			case DM_OP_SELF:                {
				stack_push(stack, self);
				break;
			}
			case DM_OP_CALL:                {
				int arguments = read8(chunk);
				dm_value func = stack_peekn(stack, arguments);
				if (arguments != func.func_val->nargs) {
					int nargs = func.func_val->nargs;
					dm_runtime_error(dm, "expected %d args, but %d args given", nargs, arguments);
				}

				while (arguments--) {
					dm_chunk_set_var(func.func_val->chunk, arguments, stack_pop(stack));
				}
				stack_pop(stack);

				dm_value ret = exec_func(dm, func, stack);
				if (dm_state_has_error(dm)) {
					return print_backtrace(dm, f);
				}
				stack_push(stack, ret);
				break;
			}
			case DM_OP_CALL_WITHPARENT:     {
				int arguments = read8(chunk);
				dm_value func = stack_peekn(stack, arguments);
				int normal_args_start = func.func_val->takes_self ? 1 : 0;
				arguments += normal_args_start;
				if (arguments != func.func_val->nargs) {
					int nargs = func.func_val->nargs;
					dm_runtime_error(dm, "expected %d args, but %d args given", nargs, arguments);
				}

				while (arguments-- > normal_args_start) {
					dm_chunk_set_var(func.func_val->chunk, arguments, stack_pop(stack));
				}
				stack_pop(stack);

				dm_value parent = stack_pop(stack);
				if (func.func_val->takes_self) {
					dm_chunk_set_var(func.func_val->chunk, 0, parent);
				}

				dm_value ret = exec_func(dm, func, stack);
				if (dm_state_has_error(dm)) {
					return print_backtrace(dm, f);
				}
				stack_push(stack, ret);
				break;
			}
			case DM_OP_NEGATE:              {
				dm_value val = stack_pop(stack);
				if (val.type == DM_TYPE_INT) {
					stack_push(stack, dm_int_negate(dm, val));
				} else if (val.type == DM_TYPE_FLOAT) {
					stack_push(stack, dm_float_negate(dm, val));
				} else {
					dm_runtime_error(dm, "Can't negate <%s>", dm_value_type_str(dm, val));
				}
				break;
			}
			case DM_OP_NOT:                 {
				dm_value val = stack_pop(stack);
				if (val.type != DM_TYPE_BOOL) {
					dm_runtime_error(dm, "Can't apply logical not to <%s>", dm_value_type_str(dm, val));
				}
				stack_push(stack, dm_value_bool(!val.bool_val));
				break;
			}
			case DM_OP_PLUS:                {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				dm_module *m = dm_state_get_module(dm, val1.type);
				stack_push(stack, m->add(dm, val1, val2));
				break;
			}
			case DM_OP_MINUS:               {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				dm_module *m = dm_state_get_module(dm, val1.type);
				stack_push(stack, m->sub(dm, val1, val2));
				break;
			}
			case DM_OP_MUL:                 {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				dm_module *m = dm_state_get_module(dm, val1.type);
				stack_push(stack, m->mul(dm, val1, val2));
				break;
			}
			case DM_OP_DIV:                 {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				dm_module *m = dm_state_get_module(dm, val1.type);
				stack_push(stack, m->div(dm, val1, val2));
				break;
			}
			case DM_OP_MOD:                 {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				dm_module *m = dm_state_get_module(dm, val1.type);
				stack_push(stack, m->mod(dm, val1, val2));
				break;
			}
			case DM_OP_NOTEQUAL:            {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				stack_push(stack, dm_value_bool(!dm_value_equals(dm, val1, val2)));
				break;
			}
			case DM_OP_EQUAL:               {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				stack_push(stack, dm_value_bool(dm_value_equals(dm, val1, val2)));
				break;
			}
			case DM_OP_LESS:                {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				dm_module *m = dm_state_get_module(dm, val1.type);
				stack_push(stack, dm_value_bool(m->compare(dm, val1, val2) < 0));
				break;
			}
			case DM_OP_LESSEQUAL:           {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				dm_module *m = dm_state_get_module(dm, val1.type);
				stack_push(stack, dm_value_bool(m->compare(dm, val1, val2) <= 0));
				break;
			}
			case DM_OP_GREATER:             {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				dm_module *m = dm_state_get_module(dm, val1.type);
				stack_push(stack, dm_value_bool(m->compare(dm, val1, val2) > 0));
				break;
			}
			case DM_OP_GREATEREQUAL:        {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				dm_module *m = dm_state_get_module(dm, val1.type);
				stack_push(stack, dm_value_bool(m->compare(dm, val1, val2) >= 0));
				break;
			}
			case DM_OP_JUMP_IF_TRUE_OR_POP: {
				dm_value val = stack_peek(stack);
				uint16_t addr = read16(chunk);
				if (!is_falsey(val)) {
					chunk->ip = addr;
				} else {
					stack_pop(stack);
				}
				break;
			}
			case DM_OP_JUMP_IF_FALSE_OR_POP:{
				dm_value val = stack_peek(stack);
				uint16_t addr = read16(chunk);
				if (is_falsey(val)) {
					chunk->ip = addr;
				} else {
					stack_pop(stack);
				}
				break;
			}
			case DM_OP_JUMP_IF_FALSE:       {
				dm_value val = stack_pop(stack);
				uint16_t addr = read16(chunk);
				if (is_falsey(val)) {
					chunk->ip = addr;
				}
				break;
			}
			case DM_OP_JUMP:                {
				uint16_t addr = read16(chunk);
				chunk->ip = addr;
				break;
			}
			case DM_OP_POP:                 {
				stack_pop(stack);
				break;
			}
			case DM_OP_RETURN:              {
				return stack_pop(stack);
				break;
			}
		}
	}
	return dm_value_nil();
}

int dm_vm_exec(dm_state *dm, char *prog, dm_value *result, bool repl) {
	dm_state_reset_error(dm);
	dm_value _nil = dm_value_nil();
	dm_value *main = repl ? dm_state_get_main(dm) : &_nil;

	if (dm_compile(dm, main, prog) != 0) {
		return 1;
	}

	dm_stack stack;
	stack_init(&stack);

	dm_value v = exec_func(dm, *main, &stack);
	if (dm_state_has_error(dm)) {
		print_backtrace(dm, *main);
	} else if (dm_debug_enabled(dm)) {
		dm_chunk_decompile(dm, main->func_val->chunk);
	}

	stack_free(&stack);
	if (result) {
		*result = v;
	}

	return dm_state_has_error(dm);
}
