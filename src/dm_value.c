#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dm.h>
#include <dm_gc.h>
#include <dm_chunk.h>

#define TABLE_INVALID_VAL ((dm_value){DM_TYPE_NIL, {.int_val = 0xDEAD}})
#define TABLE_INVALID_CODE (0xDEAD)

dm_value dm_value_nil(void) {
	return (dm_value){DM_TYPE_NIL, {0}};
}

dm_value dm_value_bool(dm_bool bool_val) {
	return (dm_value){DM_TYPE_BOOL, {.bool_val = bool_val}};
}

dm_value dm_value_int(dm_int int_val) {
	return (dm_value){DM_TYPE_INT, {.int_val = int_val}};
}

dm_value dm_value_float(dm_float float_val) {
	return (dm_value){DM_TYPE_FLOAT, {.float_val = float_val}};
}

static bool table_is_invalid_entry(dm_value v) {
	return v.type == DM_TYPE_NIL && v.int_val == TABLE_INVALID_CODE;
}

static void table_mark(dm_state *dm, struct dm_gc_obj *obj) {
	dm_table *t = (dm_table*) obj;
	dm_value *keys = (dm_value*) t->keys;
	dm_value *values = (dm_value*) t->values;
	for (int i = 0; i < t->size; i++) {
		if (table_is_invalid_entry(keys[i]) || table_is_invalid_entry(values[i])) {
			continue;
		}
		if (dm_value_is_gc_obj(keys[i])) {
			dm_gc_mark(dm, keys[i].gc_obj);
		}
		if (dm_value_is_gc_obj(values[i])) {
			dm_gc_mark(dm, values[i].gc_obj);
		}
	}
}

static void table_free(dm_state *dm, struct dm_gc_obj *obj) {
	dm_table *t = (dm_table*) obj;
	dm_value *keys = (dm_value*) t->keys;
	dm_value *values = (dm_value*) t->values;
	for (int i = 0; i < t->size; i++) {
		if (table_is_invalid_entry(keys[i]) || table_is_invalid_entry(values[i])) {
			continue;
		}
		if (dm_value_is_gc_obj(keys[i])) {
			dm_gc_free(dm, keys[i].gc_obj);
		}
		if (dm_value_is_gc_obj(values[i])) {
			dm_gc_free(dm, values[i].gc_obj);
		}
	}
	free(t->keys);
	free(t->values);
}

dm_value dm_value_table(dm_state *dm, int size) {
	dm_table *table = (dm_table*) dm_gc_malloc(dm, sizeof(dm_table), table_mark, table_free);
	table->size = size < 16 ? 16 : size;
	int bytes = sizeof(dm_value) * table->size;
	table->keys = malloc(bytes);
	table->values = malloc(bytes);
	table->parent = NULL;

	dm_value *keys = (dm_value*) table->keys;
	dm_value *values = (dm_value*) table->values;
	for (int i = 0; i < table->size; i++) {
		keys[i] = TABLE_INVALID_VAL;
		values[i] = TABLE_INVALID_VAL;
	}
	return (dm_value){DM_TYPE_TABLE, {.table_val = table}};
}

static void function_free(dm_state *dm, struct dm_gc_obj *obj) {
	(void) dm;
	dm_function *func = (dm_function*) obj;
	dm_chunk_free(func->chunk);
	free(func->chunk);
}

dm_value dm_value_function(dm_state *dm, void *chunk, int nargs, bool takes_self) {
	dm_function *func = (dm_function*) dm_gc_malloc(dm, sizeof(dm_function), NULL, function_free);
	func->chunk = chunk;
	func->nargs = nargs;
	func->takes_self = takes_self;
	return (dm_value){DM_TYPE_FUNCTION, {.func_val = func}};
}

bool dm_value_is(dm_value v, dm_type t) {
	return v.type == t;
}

static bool table_equal(dm_table *t1, dm_table *t2) {
	return t1 == t2;
}

bool dm_value_equal(dm_value v1, dm_value v2) {
	if (v1.type != v2.type) {
		return false;
	}
	switch (v1.type) {
		case DM_TYPE_NIL:      return true;
		case DM_TYPE_BOOL:     return v1.bool_val  == v2.bool_val;
		case DM_TYPE_INT:      return v1.int_val   == v2.int_val;
		case DM_TYPE_FLOAT:    return v1.float_val == v2.float_val;
		case DM_TYPE_STRING:   return dm_string_equal(v1.str_val, v2.str_val);
		case DM_TYPE_ARRAY:    return dm_array_equal(v1.arr_val, v2.arr_val);
		case DM_TYPE_TABLE:    return table_equal(v1.table_val, v2.table_val);
		case DM_TYPE_FUNCTION: return v1.func_val->chunk == v2.func_val->chunk;
		default:			   return false;
	}
}

void table_inspect(dm_table *t) {
	printf("{");

	int printed = 0;

	for (int i = 0; i < t->size; i++) {
		dm_value key = ((dm_value*) t->keys)[i];
		dm_value value = ((dm_value*) t->values)[i];
		if (key.int_val == TABLE_INVALID_CODE || value.int_val == TABLE_INVALID_CODE) {
			continue;
		}

		dm_value_inspect(key);
		printf(": ");
		dm_value_inspect(value);
		printf(", ");
		printed++;
	}

	if (printed != 0) {
		printf("\b\b");
	}

	printf("}");
}

void dm_value_inspect(dm_value v) {
	switch (v.type) {
		case DM_TYPE_NIL:      printf("nil");                         return;
		case DM_TYPE_BOOL:     printf(v.bool_val ? "true" : "false"); return;
		case DM_TYPE_INT:      printf("%ld", v.int_val);              return;
		case DM_TYPE_FLOAT:    printf("%g", v.float_val);             return;
		case DM_TYPE_STRING:   dm_string_inspect(v.str_val);          return;
		case DM_TYPE_ARRAY:    dm_array_inspect(v.arr_val);           return;
		case DM_TYPE_TABLE:    table_inspect(v.table_val);            return;
		case DM_TYPE_FUNCTION: printf("<function %p>", v.func_val);   return;
		default:			 return;
	}
}

bool dm_value_is_gc_obj(dm_value v) {
	return v.type == DM_TYPE_STRING
		|| v.type == DM_TYPE_ARRAY
		|| v.type == DM_TYPE_TABLE
		|| v.type == DM_TYPE_FUNCTION;
}

const char *dm_value_type_str(dm_value v) {
	static const char *types[] = {"nil", "bool", "integer", "float", "string", "array", "table", "function"};
	return types[v.type];
}

void dm_value_table_set(dm_value t, dm_value key, dm_value value) {
	if (!dm_value_is(t, DM_TYPE_TABLE)) {
		return;
	}

	dm_table *table = t.table_val;
	dm_value *keys = (dm_value*) table->keys;
	dm_value *values = (dm_value*) table->values;
	for (int i = 0; i < table->size; i++) {
		if (dm_value_equal(keys[i], key)) {
			values[i] = value;
			break;
		}
		if (keys[i].int_val != TABLE_INVALID_CODE && values[i].int_val != TABLE_INVALID_CODE) {
			continue;
		}
		keys[i] = key;
		values[i] = value;
		break;
	}
}

dm_value dm_value_table_get(dm_value t, dm_value key) {
	if (!dm_value_is(t, DM_TYPE_TABLE)) {
		// error
		return dm_value_nil();
	}

	dm_table *table = t.table_val;
	dm_value *keys = (dm_value*) table->keys;
	dm_value *values = (dm_value*) table->values;
	for (int i = 0; i < table->size; i++) {
		if (keys[i].int_val == TABLE_INVALID_CODE || values[i].int_val == TABLE_INVALID_CODE) {
			continue;
		}
		if (dm_value_equal(keys[i], key)) {
			return values[i];
		}
	}

	return dm_value_nil();
}
