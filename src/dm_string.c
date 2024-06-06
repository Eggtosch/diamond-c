#include <string.h>
#include <stdio.h>
#include <dm_string.h>
#include <dm.h>

struct dm_string {
	dm_gc_obj gc_header;
	uint64_t size;
	char *data;
};

static void string_free(dm_state *dm, struct dm_gc_obj *obj) {
	(void) dm;
	dm_string *str = (dm_string*) obj;
	free(str->data);
}

static dm_string *string_alloc(dm_state *dm) {
	return (dm_string*) dm_gc_malloc(dm, sizeof(dm_string), NULL, string_free);
}

dm_value dm_value_string_len(dm_state *dm, const char *s, int size) {
	dm_string *str = string_alloc(dm);
	str->size = size;
	str->data = malloc(size + 1);
	memcpy(str->data, s, size);
	str->data[size] = '\0';
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
	return s1->size == s2->size && memcmp(s1->data, s2->data, s1->size) == 0;
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
	int res = strcmp(s1, s2);
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
	dm_string *new = string_alloc(dm);
	new->size = a->size + b->size;
	new->data = malloc(new->size + 1);
	memcpy(new->data, a->data, a->size);
	memcpy(new->data + a->size, b->data, b->size);
	new->data[new->size] = '\0';
	return (dm_value){DM_TYPE_STRING, {.str_val = new}};
}

static dm_value dm_string_mul(dm_state *dm, dm_value self, dm_value other) {
	if (other.type != DM_TYPE_INT) {
		return dm_value_nil();
	}

	dm_string *a = self.str_val;
	dm_string *new = string_alloc(dm);
	new->size = a->size * other.int_val;
	new->data = malloc(new->size + 1);
	for (int i = 0; i < other.int_val; i++) {
		memcpy(new->data + i * a->size, a->data, a->size);
	}
	new->data[new->size] = '\0';
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
