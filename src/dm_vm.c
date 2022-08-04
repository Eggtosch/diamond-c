#include <stdlib.h>
#include <stdio.h>

#include <dm_vm.h>
#include <dm_compiler.h>
#include <dm_value.h>
#include <dm_chunk.h>
#include <dm_array.h>

static int stack_value_compare(int size, void *e1, void *e2) {
	(void) size;
	return dm_value_equal(*(dm_value*) e1, *(dm_value*) e2) ? 0 : 1;
}

static void stack_init(dm_array *stack) {
	dm_array_new(stack, sizeof(dm_value), stack_value_compare);
}

static void stack_free(dm_array *stack) {
	dm_array_free(stack);
}

static void stack_push(dm_array *stack, dm_value val) {
	dm_array_push(stack, (void*) &val);
}

static dm_value stack_pop(dm_array *stack) {
	dm_value result;
	if (dm_array_pop(stack, (void*) &result)) {
		return result;
	}
	return dm_value_nil();
}

static dm_value stack_peek(dm_array *stack) {
	int size = dm_array_size(stack);
	if (size <= 0) {
		return dm_value_nil();
	}
	return *(dm_value*) dm_array_get(stack, size - 1);
}

static uint8_t read8(dm_chunk *chunk) {
	uint8_t b = *(uint8_t*) dm_array_get(&chunk->code, chunk->ip);
	chunk->ip++;
	return b;
}

static uint16_t read16(dm_chunk *chunk) {
	uint16_t s = *(uint8_t*) dm_array_get(&chunk->code, chunk->ip);
	s <<= 8;
	s += *(uint8_t*) dm_array_get(&chunk->code, chunk->ip + 1);
	chunk->ip += 2;
	return s;
}

static bool is_falsey(dm_value val) {
	return dm_value_is(val, DM_TYPE_NIL) || (dm_value_is(val, DM_TYPE_BOOL) && val.bool_val == false);
}

static int exec_func(dm_value f, dm_array *stack, int nargs) {
	if (!dm_value_is(f, DM_TYPE_FUNCTION) || nargs != 0) {
		return DM_VM_RUNTIME_ERROR;
	}

	dm_chunk *chunk = (dm_chunk*) f.func_val->chunk;
	chunk->ip = 0;

	for (;;) {
		uint8_t opcode = read8(chunk);
		
		switch (opcode) {
			case DM_OP_VARSET:              {
				break;
			}
			case DM_OP_VARGET:              {
				break;
			}
			case DM_OP_FIELDSET:            {
				break;
			}
			case DM_OP_FIELDGET:            {
				break;
			}
			case DM_OP_FIELDGET_PUSHPARENT: {
				break;
			}
			case DM_OP_CONSTANT:            {
				uint16_t index = read16(chunk);
				stack_push(stack, *(dm_value*) dm_array_get(&chunk->constants, index));
				break;
			}
			case DM_OP_ARRAYLIT:            {
				break;
			}
			case DM_OP_TABLELIT:            {
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
			case DM_OP_CALL:                {
				break;
			}
			case DM_OP_CALL_WITHPARENT:     {
				break;
			}
			case DM_OP_NEGATE:              {
				dm_value val = stack_pop(stack);
				if (dm_value_is(val, DM_TYPE_INT)) {
					stack_push(stack, dm_value_int(-val.int_val));
				} else {
					return DM_VM_RUNTIME_ERROR;
				}
				break;
			}
			case DM_OP_NOT:                 {
				dm_value val = stack_pop(stack);
				if (!dm_value_is(val, DM_TYPE_BOOL)) {
					return DM_VM_RUNTIME_ERROR;
				}
				stack_push(stack, dm_value_bool(!val.bool_val));
				break;
			}
			case DM_OP_PLUS:                {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				if (dm_value_is(val1, DM_TYPE_INT) && dm_value_is(val2, DM_TYPE_INT)) {
					stack_push(stack, dm_value_int(val1.int_val + val2.int_val));
				} else {
					return DM_VM_RUNTIME_ERROR;
				}
				break;
			}
			case DM_OP_MINUS:               {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				if (dm_value_is(val1, DM_TYPE_INT) && dm_value_is(val2, DM_TYPE_INT)) {
					stack_push(stack, dm_value_int(val1.int_val - val2.int_val));
				} else {
					return DM_VM_RUNTIME_ERROR;
				}
				break;
			}
			case DM_OP_MUL:                 {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				if (dm_value_is(val1, DM_TYPE_INT) && dm_value_is(val2, DM_TYPE_INT)) {
					stack_push(stack, dm_value_int(val1.int_val * val2.int_val));
				} else {
					return DM_VM_RUNTIME_ERROR;
				}
				break;
			}
			case DM_OP_DIV:                 {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				if (dm_value_is(val1, DM_TYPE_INT) && dm_value_is(val2, DM_TYPE_INT)) {
					stack_push(stack, dm_value_int(val1.int_val / val2.int_val));
				} else {
					return DM_VM_RUNTIME_ERROR;
				}
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
				if (dm_value_is(val1, DM_TYPE_INT) && dm_value_is(val2, DM_TYPE_INT)) {
					stack_push(stack, dm_value_bool(val1.int_val < val2.int_val));
				} else {
					return DM_VM_RUNTIME_ERROR;
				}
				break;
			}
			case DM_OP_LESSEQUAL:           {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				if (dm_value_is(val1, DM_TYPE_INT) && dm_value_is(val2, DM_TYPE_INT)) {
					stack_push(stack, dm_value_bool(val1.int_val <= val2.int_val));
				} else {
					return DM_VM_RUNTIME_ERROR;
				}
				break;
			}
			case DM_OP_GREATER:             {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				if (dm_value_is(val1, DM_TYPE_INT) && dm_value_is(val2, DM_TYPE_INT)) {
					stack_push(stack, dm_value_bool(val1.int_val > val2.int_val));
				} else {
					return DM_VM_RUNTIME_ERROR;
				}
				break;
			}
			case DM_OP_GREATEREQUAL:        {
				dm_value val2 = stack_pop(stack);
				dm_value val1 = stack_pop(stack);
				if (dm_value_is(val1, DM_TYPE_INT) && dm_value_is(val2, DM_TYPE_INT)) {
					stack_push(stack, dm_value_bool(val1.int_val >= val2.int_val));
				} else {
					return DM_VM_RUNTIME_ERROR;
				}
				break;
			}
			case DM_OP_JUMP_IF_TRUE:        {
				dm_value val = stack_peek(stack);
				uint16_t addr = read16(chunk);
				if (!is_falsey(val)) {
					chunk->ip = addr;
				}
				break;
			}
			case DM_OP_JUMP_IF_FALSE:       {
				dm_value val = stack_peek(stack);
				uint16_t addr = read16(chunk);
				if (is_falsey(val)) {
					chunk->ip = addr;
				}
				break;
			}
			case DM_OP_JUMP_IF_FALSE_POP:   {
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
				dm_value_inspect(stack_peek(stack));
				printf("\n");
				return DM_VM_OK;
				break;
			}
			default: return DM_VM_RUNTIME_ERROR;
		}
	}
	return DM_VM_RUNTIME_ERROR;
}

int dm_vm_exec(dm_state *dm, char *prog) {
	if (dm_compile(dm, prog) != 0) {
		return DM_VM_COMPILE_ERROR;
	}

	dm_chunk_decompile((dm_chunk*) dm->main.func_val->chunk);

	dm_array stack;
	stack_init(&stack);

	int status = exec_func(dm->main, &stack, 0);

	stack_free(&stack);

	return status;
}

