#include <stdio.h>
#include <dm_bool.h>

static int dm_bool_compare(dm_value self, dm_value other) {
	return self.bool_val - other.bool_val;
}

static bool dm_bool_fieldget(dm_value self, const char *field, dm_value *v) {
	(void) self, (void) field, (void) v;
	return false;
}

dm_module dm_bool_init(dm_state *dm) {
	(void) dm;
	dm_module m = {0};
	m.compare = dm_bool_compare;
	m.fieldget_s = dm_bool_fieldget;
	return m;
}
