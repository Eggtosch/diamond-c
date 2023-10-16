#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dm_chunk.h>

void dm_chunk_init(dm_chunk *chunk) {
	*chunk = (dm_chunk){
		.parent = NULL,
		.codesize = 0,
		.codecapacity = 0,
		.code = NULL,
		.ip = 0,
		.constsize = 0,
		.constcapacity = 0,
		.consts = NULL,
		.varsize = 0,
		.varcapacity = 0,
		.vars = NULL,
		.lines = NULL,
		.current_line = 1
	};
	chunk->codecapacity = 128;
	chunk->code = malloc(chunk->codecapacity);
	chunk->lines = malloc(chunk->codecapacity * sizeof(int));
	chunk->constcapacity = 4;
	chunk->consts = malloc(chunk->constcapacity * sizeof(dm_value));
	chunk->varcapacity = 4;
	chunk->vars = malloc(chunk->varcapacity * sizeof(struct variable));
}

void dm_chunk_free(dm_chunk *chunk) {
	free(chunk->code);
	free(chunk->lines);
	chunk->code = NULL;
	chunk->lines = NULL;
	chunk->codesize = 0;
	chunk->codecapacity = 0;
	free(chunk->consts);
	chunk->consts = NULL;
	chunk->constsize = 0;
	chunk->constcapacity = 0;

	for (int i = 0; i < chunk->varsize; i++) {
		free((void*) chunk->vars[i].name);
		chunk->vars[i].name = NULL;
	}
	free(chunk->vars);
	chunk->vars = NULL;
	chunk->varsize = 0;
	chunk->varcapacity = 0;
	chunk->ip = 0;
}

void dm_chunk_set_parent(dm_chunk *chunk, dm_chunk *parent) {
	chunk->parent = (struct dm_chunk*) parent;
}

void dm_chunk_reset_code(dm_chunk *chunk) {
	memset(chunk->code, 0, chunk->codecapacity);
	memset(chunk->lines, 0, chunk->codecapacity * sizeof(int));
	chunk->codesize = 0;
}

int dm_chunk_current_address(dm_chunk *chunk) {
	return chunk->codesize;
}

int dm_chunk_current_line(dm_chunk *chunk) {
	return chunk->lines[chunk->ip-2];
}

void dm_chunk_set_line(dm_chunk *chunk, int line) {
	chunk->current_line = line;
}

int dm_chunk_index_of_string_constant(dm_chunk *chunk, const char *s, size_t len) {
	for (int i = 0; i < chunk->constsize; i++) {
		dm_value c = chunk->consts[i];
		if (dm_value_is(c, DM_TYPE_STRING)) {
			if ((size_t) c.str_val->size == len && strncmp(c.str_val->data, s, len) == 0) {
				return i;
			}
		}
	}
	return -1;
}

void dm_chunk_emit_constant(dm_chunk *chunk, dm_value value) {
	int index = 0;
	for (; index < chunk->constsize; index++) {
		if (dm_value_equal(chunk->consts[index], value)) {
			break;
		}
	}
	if (index >= chunk->constcapacity) {
		chunk->constcapacity *= 2;
		chunk->consts = realloc(chunk->consts, chunk->constcapacity * sizeof(dm_value));
	}
	if (index >= chunk->constsize) {
		chunk->constsize++;
	}
	chunk->consts[index] = value;
	dm_chunk_emit_arg16(chunk, DM_OP_CONSTANT, index);
}

void dm_chunk_emit_constant_i(dm_chunk *chunk, int index) {
	if (index < 0 || index >= chunk->constsize) {
		return;
	}
	dm_chunk_emit_arg16(chunk, DM_OP_CONSTANT, index);
}

static void emit_byte(dm_chunk *chunk, uint8_t byte) {
	if (chunk->codesize >= chunk->codecapacity) {
		chunk->codecapacity *= 2;
		chunk->code = realloc(chunk->code, chunk->codecapacity);
		chunk->lines = realloc(chunk->lines, chunk->codecapacity * sizeof(int));
	}
	chunk->lines[chunk->codesize] = chunk->current_line;
	chunk->code[chunk->codesize++] = byte;
}

void dm_chunk_emit(dm_chunk *chunk, dm_opcode opcode) {
	emit_byte(chunk, (uint8_t) opcode);
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

void dm_chunk_emit_arg8_arg16(dm_chunk *chunk, dm_opcode opcode, int arg8, int arg16) {
	if (arg8 >= 1 << 8 || arg16 >= 1 << 16) {
		return;
	}

	emit_byte(chunk, (uint8_t) opcode);
	emit_byte(chunk, (uint8_t) arg8);
	emit_byte(chunk, (uint16_t) arg16 >> 8);
	emit_byte(chunk, (uint16_t) arg16 & 0xff);
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
	chunk->code[addr_location] = (uint8_t) (addr >> 8);
	chunk->code[addr_location+1] = (uint8_t) (addr & 0xff);
}

int dm_chunk_add_var(dm_chunk *chunk, const char *name, int size) {
	for (int i = 0; i < chunk->varsize; i++) {
		const char *try_name = chunk->vars[i].name;
		if (strlen(try_name) == (size_t) size && strncmp(try_name, name, size) == 0) {
			return i;
		}
	}

	char *new_name = malloc(size + 1);
	memcpy(new_name, name, size);
	new_name[size] = '\0';
	if (chunk->varsize >= chunk->varcapacity) {
		chunk->varcapacity *= 2;
		chunk->vars = realloc(chunk->vars, chunk->varcapacity * sizeof(dm_value));
	}
	chunk->vars[chunk->varsize++] = (struct variable){new_name, dm_value_nil()};
	return chunk->varsize - 1;
}

int dm_chunk_find_var(dm_chunk *chunk, const char *name, int size) {
	for (int i = 0; i < chunk->varsize; i++) {
		const char *try_name = chunk->vars[i].name;
		if (strlen(try_name) == (size_t) size && strncmp(try_name, name, size) == 0) {
			return i;
		}
	}
	return -1;
}

void dm_chunk_set_var(dm_chunk *chunk, int index, dm_value v) {
	if (index < 0 || index >= chunk->varsize) {
		return;
	}
	chunk->vars[index].value = v;
}

dm_value dm_chunk_get_var(dm_chunk *chunk, int index) {
	if (index < 0 || index >= chunk->varsize) {
		return dm_value_nil();
	}
	return chunk->vars[index].value;
}


static int decompile_op(uint8_t *code) {
	uint8_t opcode = code[0];
	switch (opcode) {
		case DM_OP_VARSET:				printf("VARSET %d\n", code[1] << 8 | code[2]); return 3;
		case DM_OP_VARGET:				printf("VARGET %d\n", code[1] << 8 | code[2]); return 3;
		case DM_OP_VARGET_UP:			printf("VARGET_UP (%d) %d\n", code[1], code[2] << 8 | code[3]); return 4;
		case DM_OP_FIELDSET:			printf("FIELDSET\n"); return 1;
		case DM_OP_FIELDGET:			printf("FIELDGET\n"); return 1;
		case DM_OP_FIELDGET_PUSHPARENT:	printf("FIELDGET_PUSHPARENT\n"); return 1;

		case DM_OP_CONSTANT:			printf("CONSTANT %d\n", code[1] << 8 | code[2]); return 3;
		case DM_OP_CONSTANT_SMALLINT:	printf("CONSTANT_SMALLINT <%d>\n", code[1] << 8 | code[2]); return 3;

		case DM_OP_ARRAYLIT:			printf("ARRAYLIT %d\n", code[1] << 8 | code[2]); return 3;
		case DM_OP_TABLELIT:			printf("TABLELIT %d\n", code[1] << 8 | code[2]); return 3;
		case DM_OP_TRUE:				printf("TRUE\n"); return 1;
		case DM_OP_FALSE:				printf("FALSE\n"); return 1;
		case DM_OP_NIL:					printf("NIL\n"); return 1;
		case DM_OP_SELF:                printf("SELF\n"); return 1;

		case DM_OP_CALL:				printf("CALL %d\n", code[1]); return 2;
		case DM_OP_CALL_WITHPARENT:		printf("CALL_WITHPARENT %d\n", code[1]); return 2;

		case DM_OP_NEGATE:				printf("NEGATE\n"); return 1;
		case DM_OP_NOT:					printf("NOT\n"); return 1;
		case DM_OP_PLUS:				printf("PLUS\n"); return 1;
		case DM_OP_MINUS:				printf("MINUS\n"); return 1;
		case DM_OP_MUL:					printf("MUL\n"); return 1;
		case DM_OP_DIV:					printf("DIV\n"); return 1;
		case DM_OP_MOD:					printf("MOD\n"); return 1;
		case DM_OP_NOTEQUAL:			printf("NOTEQUAL\n"); return 1;
		case DM_OP_EQUAL:				printf("EQUAL\n"); return 1;
		case DM_OP_LESS:				printf("LESS\n"); return 1;
		case DM_OP_LESSEQUAL:			printf("LESSEQUAL\n"); return 1;
		case DM_OP_GREATER:				printf("GREATER\n"); return 1;
		case DM_OP_GREATEREQUAL:		printf("GREATEREQUAL\n"); return 1;

		case DM_OP_JUMP_IF_TRUE_OR_POP:	printf("JUMP_IF_TRUE_OR_POP %d\n", code[1] << 8 | code[2]); return 3;
		case DM_OP_JUMP_IF_FALSE_OR_POP:printf("JUMP_IF_FALSE_OR_POP %d\n", code[1] << 8 | code[2]); return 3;
		case DM_OP_JUMP_IF_FALSE:		printf("JUMP_IF_FALSE %d\n", code[1] << 8 | code[2]); return 3;
		case DM_OP_JUMP:				printf("JUMP %d\n", code[1] << 8 | code[2]); return 3;

		case DM_OP_POP:					printf("POP\n"); return 1;
		case DM_OP_RETURN:				printf("RETURN\n"); return 1;
		default: 						printf("UNKNOWN_OPCODE\n"); return 1;
	}
}

void dm_chunk_decompile(dm_chunk *chunk) {
	for (int i = 0; i < chunk->codesize;) {
		printf("%d: ", i);
		i += decompile_op(chunk->code + i);
	}

	printf("Constants:\n");
	for (int i = 0; i < chunk->constsize; i++) {
		printf("%d: ", i);
		dm_value_inspect(chunk->consts[i]);
		printf("\n");
	}

	printf("Variables:\n");
	for (int i = 0; i < chunk->varsize; i++) {
		printf("%d: %s -> ", i, chunk->vars[i].name);
		dm_value_inspect(chunk->vars[i].value);
		printf("\n");
	}
}

