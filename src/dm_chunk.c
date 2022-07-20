#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dm_chunk.h>

static void ident_array_init(dm_ident_array *variables) {
	variables->size = 0;
	variables->capacity = 0;
	variables->ident_names = NULL;
	variables->values = NULL;
}

static void ident_array_free(dm_ident_array *variables) {
	free(variables->ident_names);
	free(variables->values);
	*variables = (dm_ident_array){0, 0, NULL, NULL};
}

void dm_chunk_init(dm_chunk *chunk) {
	*chunk = (dm_chunk){0, 0, NULL, 0, {}, {}};
	dm_value_array_init(&chunk->constants);
	ident_array_init(&chunk->variables);
}

void dm_chunk_free(dm_chunk *chunk) {
	free(chunk->code);
	dm_value_array_free(&chunk->constants);
	ident_array_free(&chunk->variables);
	*chunk = (dm_chunk){0, 0, NULL, 0, {}, {}};
}

void dm_chunk_reset_code(dm_chunk *chunk) {
	memset(chunk->code, 0, chunk->capacity);
	chunk->size = 0;
}

int dm_chunk_current_address(dm_chunk *chunk) {
	return chunk->size;
}

static void emit_byte(dm_chunk *chunk, uint8_t byte) {
	if (chunk->size >= chunk->capacity) {
		if (chunk->capacity < 16) {
			chunk->capacity = 16;
		} else {
			chunk->capacity *= 2;
		}
		chunk->code = realloc(chunk->code, chunk->capacity * sizeof(uint8_t));
	}
	chunk->code[chunk->size++] = byte;
}

void dm_chunk_emit(dm_chunk *chunk, dm_opcode opcode) {
	emit_byte(chunk, (uint8_t) opcode);
}

void dm_chunk_emit_constant(dm_chunk *chunk, dm_value value) {
	int index = dm_value_array_add(&chunk->constants, value);
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
	int addr_location = chunk->size;
	emit_byte(chunk, (uint8_t) (addr >> 8));
	emit_byte(chunk, (uint8_t) (addr & 0xff));
	return addr_location;
}

void dm_chunk_patch_jump(dm_chunk *chunk, int addr_location) {
	if (chunk->size >= 1 << 16) {
		return;
	}

	uint16_t addr = chunk->size;
	chunk->code[addr_location] = (uint8_t) (addr >> 8);
	chunk->code[addr_location + 1] = (uint8_t) (addr & 0xff);
}

int dm_chunk_add_var(dm_chunk *chunk, const char *name, int size) {
	dm_ident_array *vars = &chunk->variables;
	for (int i = 0; i < vars->size; i++) {
		if (strncmp(vars->ident_names[i], name, size) == 0) {
			return i;
		}
	}

	if (vars->size >= vars->capacity) {
		if (vars->capacity < 16) {
			vars->capacity = 16;
		} else {
			vars->capacity *= 2;
		}
		vars->ident_names = realloc(vars->ident_names, vars->capacity * sizeof(char*));
		vars->values = realloc(vars->values, vars->capacity * sizeof(dm_value));
	}
	int index = vars->size++;
	char *new_var = malloc(size + 1);
	memcpy(new_var, name, size);
	new_var[size] = '\0';
	vars->ident_names[index] = new_var;
	vars->values[index] = dm_value_nil();
	return index;
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
	for (int i = 0; i < chunk->size;) {
		printf("%d: ", i);
		i += decompile_op(chunk->code + i);
	}

	printf("Constants:\n");
	for (int i = 0; i < chunk->constants.size; i++) {
		printf("%d: ", i);
		dm_value_inspect(chunk->constants.values[i]);
		printf("\n");
	}

	printf("Variables:\n");
	for (int i = 0; i < chunk->variables.size; i++) {
		printf("%d: %s -> ", i, chunk->variables.ident_names[i]);
		dm_value_inspect(chunk->variables.values[i]);
		printf("\n");
	}
}
