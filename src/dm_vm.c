#include <stdlib.h>

#include <dm_vm.h>
#include <dm_compiler.h>
#include <dm_value.h>
#include <dm_chunk.h>
#include <stdio.h>

typedef struct {
	int size;
	int capacity;
	dm_value *values;
} dm_stack;

static void stack_init(dm_stack *stack) {
	stack->size = 0;
	stack->capacity = 0;
	stack->values = NULL;
}

static void stack_free(dm_stack *stack) {
	free(stack->values);
	stack_init(stack);
}

static void stack_push(dm_stack *stack, dm_value val) {
	if (stack->size >= stack->capacity) {
		if (stack->capacity < 16) {
			stack->capacity = 16;
		} else {
			stack->capacity *= 2;
		}
		stack->values = realloc(stack->values, stack->capacity * sizeof(dm_value));
	}
	stack->values[stack->size++] = val;
}

static dm_value stack_pop(dm_stack *stack) {
	if (stack->size == 0) {
		fprintf(stderr, "Error, stack to pop from is empty!\n");
		return dm_value_nil();
	}
	return stack->values[stack->size-- - 1];
}

static dm_value stack_peek(dm_stack *stack) {
	if (stack->size == 0) {
		fprintf(stderr, "Error, stack to peek from is empty!\n");
		return dm_value_nil();
	}
	return stack->values[stack->size - 1];
}

static uint8_t read8(dm_chunk *chunk) {
	uint8_t b = chunk->code[chunk->ip];
	chunk->ip++;
	return b;
}

static uint16_t read16(dm_chunk *chunk) {
	uint16_t s = chunk->code[chunk->ip] + chunk->code[chunk->ip + 1];
	chunk->ip += 2;
	return s;
}

static bool is_falsey(dm_value val) {
	return dm_value_is(val, DM_TYPE_NIL) || (dm_value_is(val, DM_TYPE_BOOL) && val.bool_val == false);
}

static int exec_func(dm_value f, dm_stack *stack, int nargs) {
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
				stack_push(stack, chunk->constants.values[index]);
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

	dm_stack stack;
	stack_init(&stack);

	int status = exec_func(dm->main, &stack, 0);

	stack_free(&stack);

	return status;
}
