#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <dm.h>
#include <dm_gc.h>

typedef enum {
	DM_TYPE_NIL = 0,
	DM_TYPE_BOOL,
	DM_TYPE_INT,
	DM_TYPE_FLOAT,
	DM_TYPE_STRING,
	DM_TYPE_ARRAY,
	DM_TYPE_TABLE,
	DM_TYPE_FUNCTION,
	// this has always to be last enum entry
	DM_TYPE_NUM_TYPES,
} dm_type;

typedef bool     dm_bool;
typedef uint64_t dm_int;
typedef double   dm_float;
typedef struct {
	dm_gc_obj gc_header;
	int size;
	char *data;
} dm_string;
typedef struct {
	dm_gc_obj gc_header;
	int capacity;
	int size;
	void *values;
} dm_array;
typedef struct {
	dm_gc_obj gc_header;
	int size;
	void *keys;
	void *values;
	struct dm_table *parent;
} dm_table;
typedef struct {
	dm_gc_obj gc_header;
	void *chunk;
	int nargs;
	bool takes_self;
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
		dm_gc_obj   *gc_obj;
	};
} dm_value;

typedef struct {
	int      (*compare) (dm_value, dm_value);
	bool     (*fieldget_s)(dm_value, const char*, dm_value*);
	dm_value (*add)     (dm_value, dm_value);
	dm_value (*sub)     (dm_value, dm_value);
	dm_value (*mul)     (dm_value, dm_value);
	dm_value (*div)     (dm_value, dm_value);
	dm_value (*mod)     (dm_value, dm_value);
} dm_module;

dm_value dm_value_nil(void);
dm_value dm_value_bool(dm_bool bool_val);
dm_value dm_value_int(dm_int int_val);
dm_value dm_value_float(dm_float float_val);
dm_value dm_value_string_len(dm_state *dm, const char *s, int size);
dm_value dm_value_array(dm_state *dm, int capacity);
dm_value dm_value_table(dm_state *dm, int size);
dm_value dm_value_function(dm_state *dm, void *chunk, int nargs, bool takes_self);

bool dm_value_is(dm_value v, dm_type t);
bool dm_value_equal(dm_value v1, dm_value v2);
void dm_value_inspect(dm_value v);
bool dm_value_is_gc_obj(dm_value v);
const char *dm_value_type_str(dm_value v);

void dm_value_array_set(dm_value a, dm_value index, dm_value v);
dm_value dm_value_array_get(dm_value a, dm_value index);
void dm_value_table_set(dm_value t, dm_value key, dm_value value);
dm_value dm_value_table_get(dm_value t, dm_value key);
