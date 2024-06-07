#include <string.h>
#include <stdio.h>
#include <dm_string.h>
#include <dm_state.h>
#include <dm.h>

struct dm_string {
	dm_gc_obj gc_header;
	uint64_t size;
	const char *data;
};

static void string_free(dm_state *dm, struct dm_gc_obj *obj) {
	(void) dm;
	dm_string *str = (dm_string*) obj;
	free((void*) str->data);
}

static dm_string *string_alloc(dm_state *dm, bool is_const) {
	dm_gc_free_fn free_fn = is_const ? NULL : string_free;
	return (dm_string*) dm_gc_malloc(dm, sizeof(dm_string), NULL, free_fn);
}

dm_value dm_value_string_const(dm_state *dm, const char *s, int size) {
	const char *ds = dm_state_string_dedup(dm, s, size);
	dm_string *str = string_alloc(dm, true);
	str->data = ds;
	str->size = size;
	return (dm_value){DM_TYPE_STRING, {.str_val = str}};
}

size_t dm_string_size(dm_string *s) {
	return s->size;
}

const char *dm_string_c_str(dm_string *s) {
	return s->data;
}

static bool dm_string_equals(dm_state *dm, dm_value self, dm_value other) {
	(void) dm;

	if (other.type != DM_TYPE_STRING) {
		return false;
	}

	dm_string *s1 = self.str_val;
	dm_string *s2 = other.str_val;
	return s1->data == s2->data
		|| (s1->size == s2->size && memcmp(s1->data, s2->data, s1->size) == 0);
}

static void dm_string_inspect(dm_state *dm, dm_value self) {
	(void) dm;
	dm_string *s = self.str_val;
	printf("\"%.*s\"", (int) s->size, s->data);
}

static int dm_string_compare(dm_state *dm, dm_value self, dm_value other) {
	(void) dm;

	if (other.type != DM_TYPE_STRING) {
		dm_runtime_compare_mismatch(dm, self, other);
	}

	const char *s1 = self.str_val->data;
	const char *s2 = other.str_val->data;
	int res = s1 == s2 ? 0 : strcmp(s1, s2);
	return res < 0 ? -1 : res > 0 ? 1: 0;
}

static bool dm_string_fieldset(dm_state *dm, dm_value self, const char *field, dm_value v) {
	(void) dm, (void) self, (void) field, (void) v;
	return false;
}

static bool dm_string_fieldget(dm_state *dm, dm_value self, const char *field, dm_value *v) {
	(void) dm, (void) self, (void) field, (void) v;
	return false;
}

static dm_value dm_string_add(dm_state *dm, dm_value self, dm_value other) {
	if (other.type != DM_TYPE_STRING) {
		return dm_value_nil();
	}

	dm_string *a = self.str_val;
	dm_string *b = other.str_val;
	dm_string *new = string_alloc(dm, false);
	new->size = a->size + b->size;
	char *data = malloc(new->size + 1);
	memcpy(data, a->data, a->size);
	memcpy(data + a->size, b->data, b->size);
	data[new->size] = '\0';
	new->data = data;
	return (dm_value){DM_TYPE_STRING, {.str_val = new}};
}

static dm_value dm_string_mul(dm_state *dm, dm_value self, dm_value other) {
	if (other.type != DM_TYPE_INT) {
		return dm_value_nil();
	}

	dm_string *a = self.str_val;
	dm_string *new = string_alloc(dm, false);
	new->size = a->size * other.int_val;
	char *data = malloc(new->size + 1);
	for (int i = 0; i < other.int_val; i++) {
		memcpy(data + i * a->size, a->data, a->size);
	}
	data[new->size] = '\0';
	new->data = data;
	return (dm_value){DM_TYPE_STRING, {.str_val = new}};
}

dm_module dm_string_init(dm_state *dm) {
	(void) dm;
	dm_module m = dm_module_default(dm);
	m.typename = "string";
	m.equals = dm_string_equals;
	m.inspect = dm_string_inspect;
	m.fieldset_s = dm_string_fieldset;
	m.fieldget_s = dm_string_fieldget;
	m.compare = dm_string_compare;
	m.add = dm_string_add;
	m.mul = dm_string_mul;
	return m;
}
