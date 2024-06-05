#include <stdio.h>
#include <dm_bool.h>

static void dm_bool_inspect(dm_state *dm, dm_value self) {
	(void) dm;
	printf(self.bool_val ? "true" : "false");
}

static bool dm_bool_equals(dm_state *dm, dm_value self, dm_value other) {
	(void) dm;
	return other.type == DM_TYPE_BOOL && self.bool_val == other.bool_val;
}

static bool dm_bool_fieldset(dm_state *dm, dm_value self, const char *field, dm_value v) {
	(void) dm, (void) self, (void) field, (void) v;
	return false;
}

static bool dm_bool_fieldget(dm_state *dm, dm_value self, const char *field, dm_value *v) {
	(void) dm, (void) self, (void) field, (void) v;
	return false;
}

dm_module dm_bool_init(dm_state *dm) {
	(void) dm;
	dm_module m = dm_module_default(dm);
	m.typename = "bool";
	m.inspect = dm_bool_inspect;
	m.equals = dm_bool_equals;
	m.fieldset_s = dm_bool_fieldset;
	m.fieldget_s = dm_bool_fieldget;
	return m;
}
