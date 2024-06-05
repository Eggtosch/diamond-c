#include <stdio.h>
#include <dm_nil.h>

static void dm_nil_inspect(dm_state *dm, dm_value self) {
	(void) dm, (void) self;
	printf("nil");
}

static bool dm_nil_equals(dm_state *dm, dm_value self, dm_value other) {
	(void) dm, (void) self;
	return other.type == DM_TYPE_NIL;
}

static bool dm_nil_fieldset(dm_state *dm, dm_value self, const char *field, dm_value v) {
	(void) dm, (void) self, (void) field, (void) v;
	return false;
}

static bool dm_nil_fieldget(dm_state *dm, dm_value self, const char *field, dm_value *v) {
	(void) dm, (void) self, (void) field, (void) v;
	return false;
}

dm_module dm_nil_init(dm_state *dm) {
	(void) dm;
	dm_module m = dm_module_default(dm);
	m.typename = "nil";
	m.inspect = dm_nil_inspect;
	m.equals = dm_nil_equals;
	m.fieldset_s = dm_nil_fieldset;
	m.fieldget_s = dm_nil_fieldget;
	return m;
}
