#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
	DM_TYPE_NIL = 0,
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
	int capacity;
	int size;
	void *values;
} dm_array;
typedef struct {
	int size;
	void *keys;
	void *values;
	struct dm_table *parent;
} dm_table;
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
		dm_array    *arr_val;
		dm_table    *table_val;
		dm_function *func_val;
	};
} dm_value;

dm_value dm_value_nil(void);
dm_value dm_value_bool(dm_bool bool_val);
dm_value dm_value_int(dm_int int_val);
dm_value dm_value_float(dm_float float_val);
dm_value dm_value_string_len(const char *s, int size);
dm_value dm_value_array(int capacity);
dm_value dm_value_table(int size);
dm_value dm_value_function(void *chunk, int nargs);

bool dm_value_is(dm_value v, dm_type t);
bool dm_value_equal(dm_value v1, dm_value v2);
void dm_value_inspect(dm_value v);

void dm_value_array_set(dm_value a, int index, dm_value v);
void dm_value_table_set(dm_value t, dm_value key, dm_value value);

