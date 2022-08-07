#pragma once

#include <stdint.h>
#include <dm_value.h>
#include <dm_array.h>

typedef enum {
	DM_OP_VARSET,
	DM_OP_VARGET,
	DM_OP_FIELDSET,
	DM_OP_FIELDGET,
	DM_OP_FIELDGET_PUSHPARENT,

	DM_OP_CONSTANT,
	DM_OP_CONSTANT_SMALLINT,

	DM_OP_ARRAYLIT,
	DM_OP_TABLELIT,
	DM_OP_TRUE,
	DM_OP_FALSE,
	DM_OP_NIL,

	DM_OP_CALL,
	DM_OP_CALL_WITHPARENT,

	DM_OP_NEGATE,
	DM_OP_NOT,
	DM_OP_PLUS,
	DM_OP_MINUS,
	DM_OP_MUL,
	DM_OP_DIV,
	DM_OP_NOTEQUAL,
	DM_OP_EQUAL,
	DM_OP_LESS,
	DM_OP_LESSEQUAL,
	DM_OP_GREATER,
	DM_OP_GREATEREQUAL,

	DM_OP_JUMP_IF_TRUE,
	DM_OP_JUMP_IF_FALSE,
	DM_OP_JUMP_IF_FALSE_POP,
	DM_OP_JUMP,

	DM_OP_POP,
	DM_OP_RETURN
} dm_opcode;

typedef struct {
	dm_array code;
	uint64_t ip;
	dm_array constants;
	dm_array variables;
} dm_chunk;

void dm_chunk_init(dm_chunk *chunk);
void dm_chunk_free(dm_chunk *chunk);
void dm_chunk_reset_code(dm_chunk *chunk);

int  dm_chunk_current_address(dm_chunk *chunk);

void dm_chunk_emit(dm_chunk *chunk, dm_opcode opcode);
void dm_chunk_emit_constant(dm_chunk *chunk, dm_value value);
void dm_chunk_emit_arg8(dm_chunk *chunk, dm_opcode opcode, int arg8);
void dm_chunk_emit_arg16(dm_chunk *chunk, dm_opcode opcode, int arg16);
int  dm_chunk_emit_jump(dm_chunk *chunk, dm_opcode opcode, int dest);
void dm_chunk_patch_jump(dm_chunk *chunk, int addr_location);

int  dm_chunk_add_var(dm_chunk *chunk, const char *name, int size);
void dm_chunk_set_var(dm_chunk *chunk, int index, dm_value v);
dm_value dm_chunk_get_var(dm_chunk *chunk, int index);

void dm_chunk_decompile(dm_chunk *chunk);

