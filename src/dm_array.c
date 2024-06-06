#include <stdio.h>
#include <string.h>
#include <dm_array.h>
#include <dm.h>

struct dm_array {
	dm_gc_obj gc_header;
	int capacity;
	int size;
	void *values;
};

static void array_mark(dm_state *dm, struct dm_gc_obj *obj) {
	dm_array *arr = (dm_array*) obj;
	dm_value *data = (dm_value*) arr->values;
	for (int i = 0; i < arr->size; i++) {
		if (dm_value_is_gc_obj(data[i])) {
			dm_gc_mark(dm, data[i].gc_obj);
		}
	}
}

static void array_free(dm_state *dm, struct dm_gc_obj *obj) {
	dm_array *arr = (dm_array*) obj;
	dm_value *data = (dm_value*) arr->values;
	for (int i = 0; i < arr->size; i++) {
		if (dm_value_is_gc_obj(data[i])) {
			dm_gc_mark(dm, data[i].gc_obj);
		}
	}
	free(arr->values);
}

dm_value dm_value_array(dm_state *dm, int capacity) {
	dm_array *arr = (dm_array*) dm_gc_malloc(dm, sizeof(dm_array), array_mark, array_free);
	arr->capacity = capacity < 16 ? 16 : capacity;
	arr->size = capacity;
	arr->values = malloc(sizeof(dm_value) * arr->capacity);
	memset(arr->values, 0, sizeof(dm_value) * arr->capacity);
	return (dm_value){DM_TYPE_ARRAY, {.arr_val = arr}};
}

static void dm_array_inspect(dm_state *dm, dm_value self) {
	dm_array *a = self.arr_val;
	dm_value *values = a->values;

	printf("[");
	if (a->size == 0) {
		printf("]");
		return;
	}
	for (int i = 0; i < a->size - 1; i++) {
		dm_value_inspect(dm, values[i]);
		printf(", ");
	}
	dm_value_inspect(dm, values[a->size - 1]);
	printf("]");
}

void dm_value_array_set(dm_state *dm, dm_value a, dm_value index, dm_value v) {
	if (index.type != DM_TYPE_INT) {
		dm_runtime_type_mismatch(dm, DM_TYPE_INT, index);
		return;
	}

	int _index = index.int_val;
	dm_array *arr = a.arr_val;
	if (_index < 0 || _index >= arr->size) {
		dm_runtime_error(dm, "index %d out of bounds for array of length %d", _index, arr->size);
		return;
	}

	dm_value *data = (dm_value*) arr->values;
	data[_index] = v;
}

dm_value dm_value_array_get(dm_state *dm, dm_value a, dm_value index) {
	if (index.type != DM_TYPE_INT) {
		dm_runtime_type_mismatch(dm, DM_TYPE_INT, index);
	}

	int _index = index.int_val;
	dm_array *arr = a.arr_val;
	if (_index < 0 || _index >= arr->size) {
		dm_runtime_error(dm, "index %d out of bounds for array of length %d", _index, arr->size);
	}

	dm_value *data = (dm_value*) arr->values;
	return data[_index];
}

static bool dm_array_equals(dm_state *dm, dm_value self, dm_value other) {
	if (self.type != other.type) {
		return false;
	}

	dm_array *a1 = self.arr_val;
	dm_array *a2 = other.arr_val;
	if (a1->size != a2->size) {
		return false;
	}

	for (int i = 0; i < a1->size; i++) {
		dm_value v1 = ((dm_value*) a1->values)[i];
		dm_value v2 = ((dm_value*) a2->values)[i];
		if (!dm_value_equals(dm, v1, v2)) {
			return false;
		}
	}

	return true;
}

static bool dm_array_fieldset(dm_state *dm, dm_value self, const char *field, dm_value v) {
	(void) dm, (void) self, (void) field, (void) v;
	return false;
}

static bool dm_array_fieldget(dm_state *dm, dm_value self, const char *field, dm_value *v) {
	(void) dm, (void) self, (void) field, (void) v;
	return false;
}

static dm_value dm_array_add(dm_state *dm, dm_value self, dm_value other) {
	if (other.type != DM_TYPE_ARRAY) {
		dm_runtime_type_mismatch(dm, DM_TYPE_ARRAY, other);
	}

	dm_array *a = self.arr_val;
	dm_value *a_v = a->values;
	dm_array *b = other.arr_val;
	dm_value *b_v = b->values;
	dm_value arr = dm_value_array(dm, a->size + b->size);
	int i = 0;
	for (int n = 0; n < a->size; n++, i++) {
		dm_value_array_set(dm, arr, dm_value_int(i), a_v[n]);
	}

	for (int n = 0; n < b->size; n++, i++) {
		dm_value_array_set(dm, arr, dm_value_int(i), b_v[n]);
	}

	return arr;
}

static dm_value dm_array_sub(dm_state *dm, dm_value self, dm_value other) {
	if (other.type != DM_TYPE_ARRAY) {
		dm_runtime_type_mismatch(dm, DM_TYPE_ARRAY, other);
	}

	dm_array *a = self.arr_val;
	dm_value *a_v = a->values;
	dm_array *b = other.arr_val;
	dm_value *b_v = b->values;
	int items_to_delete = 0;
	for (int i = 0; i < a->size; i++) {
		for (int j = 0; j < b->size; j++) {
			if (dm_value_equals(dm, a_v[i], b_v[j])) {
				items_to_delete++;
				break;
			}
		}
	}

	dm_value new = dm_value_array(dm, a->size - items_to_delete);
	int new_i = 0;
	for (int i = 0; i < a->size; i++) {
		bool should_delete = false;
		for (int j = 0; j < b->size; j++) {
			if (dm_value_equals(dm, a_v[i], b_v[j])) {
				should_delete = true;
				break;
			}
		}

		if (!should_delete) {
			dm_value_array_set(dm, new, dm_value_int(new_i), a_v[i]);
			new_i++;
		}
	}

	return new;
}

static dm_value dm_array_mul(dm_state *dm, dm_value self, dm_value other) {
	if (other.type != DM_TYPE_INT) {
		dm_runtime_type_mismatch(dm, DM_TYPE_INT, other);
	}

	dm_array *a = self.arr_val;
	dm_value *a_v = a->values;
	dm_value new = dm_value_array(dm, a->size * other.int_val);
	int new_i = 0;
	for (int i = 0; i < (int) other.int_val; i++) {
		for (int n = 0; n < a->size; n++, new_i++) {
			dm_value_array_set(dm, new, dm_value_int(new_i), a_v[n]);
		}
	}

	return new;
}

dm_module dm_array_init(dm_state *dm) {
	(void) dm;
	dm_module m = dm_module_default(dm);
	m.typename = "array";
	m.inspect = dm_array_inspect;
	m.equals = dm_array_equals;
	m.fieldset_s = dm_array_fieldset;
	m.fieldget_s = dm_array_fieldget;
	m.add = dm_array_add;
	m.sub = dm_array_sub;
	m.mul = dm_array_mul;
	return m;
}
