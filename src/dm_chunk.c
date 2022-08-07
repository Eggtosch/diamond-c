#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dm_chunk.h>

struct variable {
	const char *name;
	dm_value value;
};

static int value_compare(int size, void *e1, void *e2) {
	(void) size;
	return dm_value_equal(*(dm_value*) e1, *(dm_value*) e2) ? 0 : 1;
}

static int byte_compare(int size, void *e1, void *e2) {
	(void) size;
	return *(uint8_t*) e1 == *(uint8_t*) e2;
}

static int variable_compare(int size, void *e1, void *e2) {
	(void) size;
	struct variable *v1 = (struct variable*) e1;
	struct variable *v2 = (struct variable*) e2;
	return strcmp(v1->name, v2->name) == 0;
}

void dm_chunk_init(dm_chunk *chunk) {
	*chunk = (dm_chunk){{}, 0, {}, {}};
	dm_array_new(&chunk->code, sizeof(uint8_t), byte_compare);
	dm_array_new(&chunk->constants, sizeof(dm_value), value_compare);
	dm_array_new(&chunk->variables, sizeof(struct variable), variable_compare);
}

void dm_chunk_free(dm_chunk *chunk) {
	dm_array_free(&chunk->code);
	dm_array_free(&chunk->constants);
	
	int n_variables = dm_array_size(&chunk->variables);
	for (int i = 0; i < n_variables; i++) {
		struct variable *v = (struct variable*) dm_array_get(&chunk->variables, i);
		free((void*) v->name);
		v->name = NULL;
	}
	dm_array_free(&chunk->variables);
	*chunk = (dm_chunk){{}, 0, {}, {}};
}

void dm_chunk_reset_code(dm_chunk *chunk) {
	dm_array_clear(&chunk->code);
}

int dm_chunk_current_address(dm_chunk *chunk) {
	return dm_array_size(&chunk->code);
}

static void emit_byte(dm_chunk *chunk, uint8_t byte) {
	dm_array_push(&chunk->code, (void*) &byte);
}

void dm_chunk_emit(dm_chunk *chunk, dm_opcode opcode) {
	emit_byte(chunk, (uint8_t) opcode);
}

void dm_chunk_emit_constant(dm_chunk *chunk, dm_value value) {
	int index = dm_array_index(&chunk->constants, (void*) &value);
	if (index == -1) {
		index = dm_array_push(&chunk->constants, (void*) &value);
	}
	dm_chunk_emit_arg16(chunk, DM_OP_CONSTANT, index);
}

void dm_chunk_emit_arg8(dm_chunk *chunk, dm_opcode opcode, int arg8) {
	if (arg8 >= 1 << 8) {
		return;
	}

	emit_byte(chunk, (uint8_t) opcode);
	emit_byte(chunk, (uint8_t) arg8);
}

void dm_chunk_emit_arg16(dm_chunk *chunk, dm_opcode opcode, int arg16) {
	if (arg16 >= 1 << 16) {
		return;
	}

	uint16_t arg = (uint16_t) arg16;
	emit_byte(chunk, (uint8_t) opcode);
	emit_byte(chunk, arg >> 8);
	emit_byte(chunk, arg & 0xff);
}

int dm_chunk_emit_jump(dm_chunk *chunk, dm_opcode opcode, int dest) {
	if (dest >= 1 << 16) {
		return 0;
	}

	uint16_t addr = (uint16_t) dest;
	emit_byte(chunk, (uint8_t) opcode);
	int addr_location = dm_chunk_current_address(chunk);
	emit_byte(chunk, (uint8_t) (addr >> 8));
	emit_byte(chunk, (uint8_t) (addr & 0xff));
	return addr_location;
}

void dm_chunk_patch_jump(dm_chunk *chunk, int addr_location) {
	if (dm_chunk_current_address(chunk) >= 1 << 16) {
		return;
	}

	uint16_t addr = dm_chunk_current_address(chunk);
	*(uint8_t*) dm_array_get(&chunk->code, addr_location) = (uint8_t) (addr >> 8);
	*(uint8_t*) dm_array_get(&chunk->code, addr_location+1) = (uint8_t) (addr & 0xff);
}

int dm_chunk_add_var(dm_chunk *chunk, const char *name, int size) {
	int n_variables = dm_array_size(&chunk->variables);
	for (int i = 0; i < n_variables; i++) {
		const char *try_name = ((struct variable*) dm_array_get(&chunk->variables, i))->name;
		if (strncmp(try_name, name, size) == 0) {
			return i;
		}
	}

	char *new_name = malloc(size + 1);
	memcpy(new_name, name, size);
	new_name[size] = '\0';
	struct variable new_var = {new_name, dm_value_nil()};
	return dm_array_push(&chunk->variables, (void*) &new_var);
}

void dm_chunk_set_var(dm_chunk *chunk, int index, dm_value v) {
	struct variable *var = dm_array_get(&chunk->variables, index);
	if (var != NULL) {
		var->value = v;
	}
}

dm_value dm_chunk_get_var(dm_chunk *chunk, int index) {
	struct variable *var = dm_array_get(&chunk->variables, index);
	if (var != NULL) {
		return var->value;
	}
	return dm_value_nil();
}


static int decompile_op(uint8_t *code) {
	uint8_t opcode = code[0];
	switch (opcode) {
		case DM_OP_VARSET:				printf("VAR_SET %d\n", code[1] << 8 | code[2]); return 3;
		case DM_OP_VARGET:				printf("VAR_GET %d\n", code[1] << 8 | code[2]); return 3;
		case DM_OP_FIELDSET:			printf("FIELD_SET\n"); return 1;
		case DM_OP_FIELDGET:			printf("FIELD_GET\n"); return 1;
		case DM_OP_FIELDGET_PUSHPARENT:	printf("FIELDGET_PUSHPARENT\n"); return 1;

		case DM_OP_CONSTANT:			printf("CONSTANT %d\n", code[1] << 8 | code[2]); return 3;
		case DM_OP_CONSTANT_SMALLINT:	printf("CONSTANT SMALL INT <%d>\n", code[1] << 8 | code[2]); return 3;

		case DM_OP_ARRAYLIT:			printf("ARRAYLIT\n"); return 1;
		case DM_OP_TABLELIT:			printf("TABLELIT\n"); return 1;
		case DM_OP_TRUE:				printf("TRUE\n"); return 1;
		case DM_OP_FALSE:				printf("FALSE\n"); return 1;
		case DM_OP_NIL:					printf("NIL\n"); return 1;

		case DM_OP_CALL:				printf("CALL %d\n", code[1]); return 2;
		case DM_OP_CALL_WITHPARENT:		printf("CALL_WITHPARENT %d\n", code[1]); return 2;

		case DM_OP_NEGATE:				printf("NEGATE\n"); return 1;
		case DM_OP_NOT:					printf("NOT\n"); return 1;
		case DM_OP_PLUS:				printf("PLUS\n"); return 1;
		case DM_OP_MINUS:				printf("MINUS\n"); return 1;
		case DM_OP_MUL:					printf("MUL\n"); return 1;
		case DM_OP_DIV:					printf("DIV\n"); return 1;
		case DM_OP_NOTEQUAL:			printf("NOTEQUAL\n"); return 1;
		case DM_OP_EQUAL:				printf("EQUAL\n"); return 1;
		case DM_OP_LESS:				printf("LESS\n"); return 1;
		case DM_OP_LESSEQUAL:			printf("LESSEQUAL\n"); return 1;
		case DM_OP_GREATER:				printf("GREATER\n"); return 1;
		case DM_OP_GREATEREQUAL:		printf("GREATEREQUAL\n"); return 1;

		case DM_OP_JUMP_IF_TRUE:		printf("JUMP IF TRUE %d\n", code[1] << 8 | code[2]); return 3;
		case DM_OP_JUMP_IF_FALSE:		printf("JUMP IF FALSE %d\n", code[1] << 8 | code[2]); return 3;
		case DM_OP_JUMP_IF_FALSE_POP:	printf("JUMP IF FALSE POP %d\n", code[1] << 8 | code[2]); return 3;
		case DM_OP_JUMP:				printf("JUMP %d\n", code[1] << 8 | code[2]); return 3;

		case DM_OP_POP:					printf("POP\n"); return 1;
		case DM_OP_RETURN:				printf("RETURN\n"); return 1;
		default: 						printf("UNKNOWN OPCODE\n"); return 1;
	}
}

void dm_chunk_decompile(dm_chunk *chunk) {
	for (int i = 0; i < dm_array_size(&chunk->code);) {
		printf("%d: ", i);
		i += decompile_op((uint8_t*) dm_array_get(&chunk->code, i));
	}

	printf("Constants:\n");
	int n_consts = dm_array_size(&chunk->constants);
	for (int i = 0; i < n_consts; i++) {
		printf("%d: ", i);
		dm_value_inspect(*(dm_value*) dm_array_get(&chunk->constants, i));
		printf("\n");
	}

	printf("Variables:\n");
	int n_variables = dm_array_size(&chunk->variables);
	for (int i = 0; i < n_variables; i++) {
		struct variable *var = (struct variable*) dm_array_get(&chunk->variables, i);
		printf("%d: %s -> ", i, var->name);
		dm_value_inspect(var->value);
		printf("\n");
	}
}

