#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <dm_gc.h>

typedef struct dm_state dm_state;

typedef enum {
	DM_TYPE_NIL = 0,
	DM_TYPE_BOOL,
	DM_TYPE_INT,
	DM_TYPE_FLOAT,
	DM_TYPE_STRING,
	DM_TYPE_ARRAY,
	DM_TYPE_TABLE,
	DM_TYPE_FUNCTION,
} dm_type;

#define DM_TYPE_NUM_TYPES 8

typedef bool     dm_bool;
typedef int64_t dm_int;
typedef double   dm_float;
typedef struct dm_string dm_string;
typedef struct dm_array dm_array;
typedef struct dm_table dm_table;
typedef struct dm_function dm_function;

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
	// required
	const char *typename;
	void     (*inspect)   (dm_state*, dm_value);
	bool     (*equals)    (dm_state*, dm_value, dm_value);
	bool     (*fieldset_s)(dm_state*, dm_value, const char*, dm_value);
	bool     (*fieldget_s)(dm_state*, dm_value, const char*, dm_value*);
	// optional
	int      (*compare)   (dm_state*, dm_value, dm_value);
	dm_value (*add)       (dm_state*, dm_value, dm_value);
	dm_value (*sub)       (dm_state*, dm_value, dm_value);
	dm_value (*mul)       (dm_state*, dm_value, dm_value);
	dm_value (*div)       (dm_state*, dm_value, dm_value);
	dm_value (*mod)       (dm_state*, dm_value, dm_value);
} dm_module;

dm_value dm_value_nil(void);
dm_value dm_value_bool(dm_bool bool_val);
dm_value dm_value_int(dm_int int_val);
dm_value dm_value_float(dm_float float_val);
dm_value dm_value_string_const(dm_state *dm, const char *s, int size);
dm_value dm_value_array(dm_state *dm, int capacity);
dm_value dm_value_table(dm_state *dm, int size);
dm_value dm_value_function(dm_state *dm, void *chunk, int nargs, bool takes_self);

bool dm_value_equals(dm_state *dm, dm_value v1, dm_value v2);
void dm_value_inspect(dm_state *dm, dm_value v);
bool dm_value_is_gc_obj(dm_value v);
const char *dm_value_type_str(dm_state *dm, dm_value v);
dm_module dm_module_default(dm_state *dm);
