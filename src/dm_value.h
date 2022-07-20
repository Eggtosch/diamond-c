#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
	DM_TYPE_NIL,
	DM_TYPE_BOOL,
	DM_TYPE_INT,
	DM_TYPE_FLOAT,
	DM_TYPE_STRING,
	DM_TYPE_ARRAY,
	DM_TYPE_TABLE,
	DM_TYPE_FUNCTION
} dm_type;

typedef bool     dm_bool;
typedef uint64_t dm_int;
typedef double   dm_float;
typedef struct {
	int size;
	char *data;
} dm_string;
typedef struct {
	void *chunk;
	int nargs;
} dm_function;

typedef struct {
	dm_type type;
	union {
		dm_bool      bool_val;
		dm_int       int_val;
		dm_float     float_val;
		dm_string   *str_val;
		dm_function *func_val;
	};
} dm_value;

dm_value dm_value_nil(void);
dm_value dm_value_bool(dm_bool bool_val);
dm_value dm_value_int(dm_int int_val);
dm_value dm_value_float(dm_float float_val);
dm_value dm_value_string_len(const char *s, int size);
dm_value dm_value_function(void *chunk, int nargs);

bool dm_value_is(dm_value v, dm_type t);
bool dm_value_equal(dm_value v1, dm_value v2);
void dm_value_inspect(dm_value v);

typedef struct {
	int size;
	int capacity;
	dm_value *values;
} dm_value_array;

void dm_value_array_init(dm_value_array *array);
void dm_value_array_free(dm_value_array *array);
int  dm_value_array_add(dm_value_array *array, dm_value v);
