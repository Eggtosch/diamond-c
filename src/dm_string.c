#include <string.h>
#include <dm_string.h>

static int dm_string_compare(dm_value self, dm_value other) {
	const char *s1 = self.str_val->data;
	const char *s2 = other.str_val->data;
	int res = strcmp(s1, s2);
	return res < 0 ? -1 : res > 0 ? 1: 0;
}

static bool dm_string_fieldset(dm_value self, const char *field, dm_value v) {
	(void) self, (void) field, (void) v;
	return false;
}

static bool dm_string_fieldget(dm_value self, const char *field, dm_value *v) {
	(void) self, (void) field, (void) v;
	return false;
}

static dm_value dm_string_add(dm_value self, dm_value other) {
	return dm_value_nil();
}

static dm_value dm_string_mul(dm_value self, dm_value other) {
	return dm_value_nil();
}

dm_module dm_string_init(dm_state *dm) {
	(void) dm;
	dm_module m = {0};
	m.compare = dm_string_compare;
	m.fieldset_s = dm_string_fieldset;
	m.fieldget_s = dm_string_fieldget;
	m.add = dm_string_add;
	m.mul = dm_string_mul;
	return m;
}
