#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dm_value.h>

dm_value dm_value_nil(void) {
	return (dm_value){DM_TYPE_NIL, {}};
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

dm_value dm_value_string_len(const char *s, int size) {
	dm_string *str = malloc(sizeof(dm_string));
	str->size = size;
	str->data = malloc(size + 1);
	memcpy(str->data, s, size);
	str->data[size] = '\0';
	return (dm_value){DM_TYPE_STRING, {.str_val = str}};
}

dm_value dm_value_array(int capacity) {
	dm_varray *arr = malloc(sizeof(dm_varray));
	arr->capacity = capacity;
	arr->size = capacity;
	arr->values = malloc(sizeof(dm_value) * arr->capacity);
	memset(arr->values, 0, sizeof(dm_value) * arr->capacity);
	return (dm_value){DM_TYPE_ARRAY, {.arr_val = arr}};
}

dm_value dm_value_function(void *chunk, int nargs) {
	dm_function *func = malloc(sizeof(dm_function));
	func->chunk = chunk;
	func->nargs = nargs;
	return (dm_value){DM_TYPE_FUNCTION, {.func_val = func}};
}

bool dm_value_is(dm_value v, dm_type t) {
	return v.type == t;
}

static bool string_equal(dm_string *s1, dm_string *s2) {
	return s1->size == s2->size && memcmp(s1->data, s2->data, s1->size) == 0;
}

static bool array_equal(dm_varray *a1, dm_varray *a2) {
	if (a1->size != a2->size) {
		return false;
	}
	for (int i = 0; i < a1->size; i++) {
		dm_value v1 = ((dm_value*) a1->values)[i];
		dm_value v2 = ((dm_value*) a2->values)[i];
		if (!dm_value_equal(v1, v2)) {
			return false;
		}
	}
	return true;
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
		case DM_TYPE_STRING:   return string_equal(v1.str_val, v2.str_val);
		case DM_TYPE_ARRAY:    return array_equal(v1.arr_val, v2.arr_val);
		case DM_TYPE_FUNCTION: return v1.func_val->chunk == v2.func_val->chunk;
		default:			   return false;
	}
}

void array_inspect(dm_varray *a) {
	printf("[");
	if (a->size == 0) {
		printf("]");
		return;
	}
	for (int i = 0; i < a->size - 1; i++) {
		dm_value v = ((dm_value*) a->values)[i];
		dm_value_inspect(v);
		printf(", ");
	}
	dm_value v = ((dm_value*) a->values)[a->size - 1];
	dm_value_inspect(v);
	printf("]");
}

void dm_value_inspect(dm_value v) {
	switch (v.type) {
		case DM_TYPE_NIL:      printf("nil");                                        return;
		case DM_TYPE_BOOL:     printf(v.bool_val ? "true" : "false");                return;
		case DM_TYPE_INT:      printf("%ld", v.int_val);                             return;
		case DM_TYPE_FLOAT:    printf("%g", v.float_val);                            return;
		case DM_TYPE_STRING:   printf("\"%.*s\"", v.str_val->size, v.str_val->data); return;
		case DM_TYPE_ARRAY:    array_inspect(v.arr_val);                             return;
		case DM_TYPE_FUNCTION: printf("<function %p>", v.func_val);                  return;
		default:			 return;
	}
}

void dm_value_array_set(dm_value a, int index, dm_value v) {
	if (!dm_value_is(a, DM_TYPE_ARRAY)) {
		return;
	}

	dm_varray *arr = a.arr_val;
	if (index >= arr->size) {
		return;
	}

	dm_value *data = (dm_value*) arr->values;
	data[index] = v;
}

