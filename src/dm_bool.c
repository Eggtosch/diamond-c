#include <stdio.h>
#include <dm_bool.h>

static int dm_bool_compare(dm_value self, dm_value other) {
	return self.bool_val - other.bool_val;
}

static bool dm_bool_fieldget(dm_value self, dm_value field, dm_value *v) {
	(void) self, (void) field, (void) v;
	return false;
}

static bool dm_bool_fieldset(dm_value self, dm_value field, dm_value v) {
	(void) self, (void) field, (void) v;
	return false;
}

dm_module dm_bool_init(dm_state *dm) {
	(void) dm;
	dm_module m = {0};
	m.compare = dm_bool_compare;
	m.fieldget = dm_bool_fieldget;
	m.fieldset = dm_bool_fieldset;
	return m;
}
