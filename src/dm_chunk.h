#pragma once

#include <stdint.h>
#include <dm_value.h>
#include <dm_array.h>

typedef enum {
	DM_OP_VARSET,				// op8 index16 | [value] -> []
	DM_OP_VARGET,				// op8 index16 | [] -> []
	DM_OP_FIELDSET,				// op8 | [table, field, value] -> [value]
	DM_OP_FIELDGET,				// op8 | [table, field] -> [value]
	DM_OP_FIELDGET_PUSHPARENT,	// op8 | [table, field] -> [table, value]

	DM_OP_CONSTANT,				// op8 index16 | [] -> [value]
	DM_OP_CONSTANT_SMALLINT,	// op8 imm16 | [] -> [value]

	DM_OP_ARRAYLIT,				// op8 imm16 | [v1, ..., vn] -> [value]
	DM_OP_TABLELIT,				// op8 imm16 | [k1, v1, ..., kn, vn] -> [value]
	DM_OP_TRUE,					// op8 | [] -> [value]
	DM_OP_FALSE,				// op8 | [] -> [value]
	DM_OP_NIL,					// op8 | [] -> [value]

	DM_OP_CALL,					// op8 nargs8 | [func, arg1, ..., argn] -> [result]
	DM_OP_CALL_WITHPARENT,		// op8 nargs8 | [parent, func, arg1, ..., argn] -> [result]

	DM_OP_NEGATE,				// op8 | [value] -> [value]
	DM_OP_NOT,					// op8 | [value] -> [value]
	DM_OP_PLUS,					// op8 | [value1, value2] -> [value]
	DM_OP_MINUS,				// op8 | [value1, value2] -> [value]
	DM_OP_MUL,					// op8 | [value1, value2] -> [value]
	DM_OP_DIV,					// op8 | [value1, value2] -> [value]
	DM_OP_NOTEQUAL,				// op8 | [value1, value2] -> [value]
	DM_OP_EQUAL,				// op8 | [value1, value2] -> [value]
	DM_OP_LESS,					// op8 | [value1, value2] -> [value]
	DM_OP_LESSEQUAL,			// op8 | [value1, value2] -> [value]
	DM_OP_GREATER,				// op8 | [value1, value2] -> [value]
	DM_OP_GREATEREQUAL,			// op8 | [value1, value2] -> [value]

	DM_OP_JUMP_IF_TRUE_OR_POP,	// op8 addr16 | cond ? [cond] -> [cond] : [cond] -> []
	DM_OP_JUMP_IF_FALSE_OR_POP,	// op8 addr16 | cond ? [cond] -> [] : [cond] -> [cond]
	DM_OP_JUMP_IF_FALSE,		// op8 addr16 | [cond] -> []
	DM_OP_JUMP,					// op8 addr16 | [] -> []

	DM_OP_POP,					// op8 | [value] -> []
	DM_OP_RETURN				// op8 | [value] -> []
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

