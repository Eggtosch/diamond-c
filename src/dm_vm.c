#define __STDC_WANT_LIB_EXT2__ 1
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <dm_vm.h>
#include <dm_compiler.h>
#include <dm_value.h>
#include <dm_chunk.h>
#include <dm_int.h>
#include <dm_float.h>

typedef dm_value (dm_value_binary_fn)(dm_value, dm_value);
typedef int (dm_value_cmp_fn)(dm_value, dm_value);

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
		return dm_value_nil();
	}
	stack->size--;
	return stack->data[stack->size];
}

static dm_value stack_peek(dm_stack *stack) {
	if (stack->size == 0) {
		return dm_value_nil();
	}
	return stack->data[stack->size - 1];
}

static dm_value stack_peekn(dm_stack *stack, int n) {
	if (stack->size <= n) {
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
	return dm_value_is(val, DM_TYPE_NIL) || (dm_value_is(val, DM_TYPE_BOOL) && val.bool_val == false);
}

static dm_value runtime_error(dm_state *dm, dm_chunk *chunk, const char *message, ...) {
	char *fmt;
	va_list args;

	printf("Error in line %d: ", dm_chunk_current_line(chunk));

	va_start(args, message);
	vasprintf(&fmt, message, args);

	dm_state_set_error(dm, fmt);

	free(fmt);
	va_end(args);
	return dm_value_nil();
}

static dm_value no_method_error(dm_state *dm, dm_chunk *chunk, const char *method, dm_value v) {
	const char *ty = dm_value_type_str(v);
	return runtime_error(dm, chunk, "Unknown method '%s' for <%s>", method, ty);
}

static dm_value compare_mismatch(dm_state *dm, dm_chunk *chunk, dm_value v1, dm_value v2) {
	const char *msg = "Can't compare <%s> and <%s>";
	const char *ty1 = dm_value_type_str(v1);
	const char *ty2 = dm_value_type_str(v2);
	return runtime_error(dm, chunk, msg, ty1, ty2);
}

static dm_value print_backtrace(dm_chunk *chunk, dm_value f) {
	printf("    in ");
	dm_value_inspect(f);
	printf("(%d)\n", dm_chunk_current_line(chunk));
	return dm_value_nil();
}

static dm_value exec_func(dm_state *dm, dm_value f, dm_stack *stack) {
	dm_chunk *chunk = (dm_chunk*) f.func_val->chunk;
	chunk->ip = 0;

	dm_value self = dm_value_nil();
	if (f.func_val->takes_self && f.func_val->nargs > 0) {
		self = dm_chunk_get_var(chunk, 0);
	}

	for (;;) {
		uint8_t opcode = read8(chunk);
		
		switch (opcode) {
			case DM_OP_VARSET:              {
				int index = read16(chunk);
				dm_value v = stack_peek(stack);
				dm_chunk_set_var(chunk, index, v);
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
						return runtime_error(dm, chunk, "Can't get upvalue from up chunk %d", ups);
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
				if (dm_value_is(table, DM_TYPE_ARRAY)) {
					dm_value_array_set(table, field, v);
				} else if (dm_value_is(table, DM_TYPE_TABLE)) {
					dm_value_table_set(table, field, v);
				} else {
					const char *msg = "Can't set field of <%s>, expected <array> or <table>";
					runtime_error(dm, chunk, msg, dm_value_type_str(field));
				}
				stack_push(stack, v);
				break;
			}
			case DM_OP_FIELDGET:            {
				dm_value field = stack_pop(stack);
				dm_value table = stack_pop(stack);
				dm_value v = dm_value_nil();
				if (dm_value_is(table, DM_TYPE_ARRAY)) {
					v = dm_value_array_get(table, field);
				} else if (dm_value_is(table, DM_TYPE_TABLE)) {
					v = dm_value_table_get(table, field);
				} else {
					const char *msg = "Can't get field of <%s>, expected <array> or <table>";
					return runtime_error(dm, chunk, msg, dm_value_type_str(table));
				}
				stack_push(stack, v);
				break;
			}
			case DM_OP_FIELDGET_PUSHPARENT: {
				dm_value field = stack_pop(stack);
				dm_value table = stack_peek(stack);
				dm_value v = dm_value_nil();
				if (dm_value_is(table, DM_TYPE_ARRAY)) {
					v = dm_value_array_get(table, field);
				} else if (dm_value_is(table, DM_TYPE_TABLE)) {
					v = dm_value_table_get(table, field);
				} else {
					const char *msg = "Can't get field of <%s>, expected <array> or <table>";
					return runtime_error(dm, chunk, msg, dm_value_type_str(table));
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
					dm_value_array_set(arr, dm_value_int(elements), stack_pop(stack));
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
					dm_value_table_set(tab, key, value);
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
					return runtime_error(dm, chunk, "expected %d args, but %d args given", nargs, arguments);
				}

				while (arguments--) {
					dm_chunk_set_var(func.func_val->chunk, arguments, stack_pop(stack));
				}
				stack_pop(stack);

				dm_value ret = exec_func(dm, func, stack);
				if (dm_state_has_error(dm)) {
					return print_backtrace(chunk, f);
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
					return runtime_error(dm, chunk, "expected %d args, but %d args given", nargs, arguments);
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
					return print_backtrace(chunk, f);
				}
				stack_push(stack, ret);
				break;
			}
			case DM_OP_NEGATE:              {
				dm_value val = stack_pop(stack);
				if (val.type == DM_TYPE_INT) {
					stack_push(stack, dm_int_negate(val));
				} else if (val.type == DM_TYPE_FLOAT) {
					stack_push(stack, dm_float_negate(val));
				} else {
					return dm_value_nil();
				}
				break;
			}
			case DM_OP_NOT:                 {
				dm_value val = stack_pop(stack);
				if (!dm_value_is(val, DM_TYPE_BOOL)) {
					return dm_value_nil();
				}
				stack_push(stack, dm_value_bool(!val.bool_val));
				break;
			}
			case DM_OP_PLUS:                {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				dm_module *m = dm_state_get_module(dm, val1.type);
				dm_value_binary_fn *fn = m->add;
				if (fn == NULL) {
					return no_method_error(dm, chunk, "+", val1);
				}
				stack_push(stack, fn(val1, val2));
				break;
			}
			case DM_OP_MINUS:               {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				dm_module *m = dm_state_get_module(dm, val1.type);
				dm_value_binary_fn *fn = m->sub;
				if (fn == NULL) {
					return no_method_error(dm, chunk, "-", val1);
				}
				stack_push(stack, fn(val1, val2));
				break;
			}
			case DM_OP_MUL:                 {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				dm_module *m = dm_state_get_module(dm, val1.type);
				dm_value_binary_fn *fn = m->mul;
				if (fn == NULL) {
					return no_method_error(dm, chunk, "*", val1);
				}
				stack_push(stack, fn(val1, val2));
				break;
			}
			case DM_OP_DIV:                 {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				dm_module *m = dm_state_get_module(dm, val1.type);
				dm_value_binary_fn *fn = m->div;
				if (fn == NULL) {
					return no_method_error(dm, chunk, "/", val1);
				}
				stack_push(stack, fn(val1, val2));
				break;
			}
			case DM_OP_MOD:                 {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				dm_module *m = dm_state_get_module(dm, val1.type);
				dm_value_binary_fn *fn = m->mod;
				if (fn == NULL) {
					return no_method_error(dm, chunk, "%", val1);
				}
				stack_push(stack, fn(val1, val2));
				break;
			}
			case DM_OP_NOTEQUAL:            {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				stack_push(stack, dm_value_bool(!dm_value_equal(val1, val2)));
				break;
			}
			case DM_OP_EQUAL:               {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				stack_push(stack, dm_value_bool(dm_value_equal(val1, val2)));
				break;
			}
			case DM_OP_LESS:                {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				if (val1.type != val2.type) {
					return compare_mismatch(dm, chunk, val1, val2);
				}
				dm_module *m = dm_state_get_module(dm, val1.type);
				dm_value_cmp_fn *fn = m->compare;
				stack_push(stack, dm_value_bool(fn(val1, val2) < 0));
				break;
			}
			case DM_OP_LESSEQUAL:           {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				if (val1.type != val2.type) {
					return compare_mismatch(dm, chunk, val1, val2);
				}
				dm_module *m = dm_state_get_module(dm, val1.type);
				dm_value_cmp_fn *fn = m->compare;
				stack_push(stack, dm_value_bool(fn(val1, val2) <= 0));
				break;
			}
			case DM_OP_GREATER:             {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				if (val1.type != val2.type) {
					return compare_mismatch(dm, chunk, val1, val2);
				}
				dm_module *m = dm_state_get_module(dm, val1.type);
				dm_value_cmp_fn *fn = m->compare;
				stack_push(stack, dm_value_bool(fn(val1, val2) > 0));
				break;
			}
			case DM_OP_GREATEREQUAL:        {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				if (val1.type != val2.type) {
					return compare_mismatch(dm, chunk, val1, val2);
				}
				dm_module *m = dm_state_get_module(dm, val1.type);
				dm_value_cmp_fn *fn = m->compare;
				stack_push(stack, dm_value_bool(fn(val1, val2) >= 0));
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
			default: return dm_value_nil();
		}
	}
	return dm_value_nil();
}

dm_value dm_vm_exec(dm_state *dm, char *prog) {
	dm_state_set_error(dm, NULL);
	if (dm_compile(dm, prog) != 0) {
		return dm_value_nil();
	}

	dm_value main = *(dm_value*) dm_state_get_main(dm);
	if (!dm_value_is(main, DM_TYPE_FUNCTION)) {
		return dm_value_nil();
	}

	dm_stack stack;
	stack_init(&stack);

	dm_chunk_decompile(main.func_val->chunk);
	dm_value v = exec_func(dm, main, &stack);

	stack_free(&stack);

	return v;
}

