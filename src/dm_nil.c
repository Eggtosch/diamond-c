#include <stdio.h>
#include <dm_nil.h>

static int dm_nil_compare(dm_value self, dm_value other) {
	(void) self;
	(void) other;
	return 0;
}

static bool dm_nil_fieldset(dm_value self, const char *field, dm_value v) {
	(void) self, (void) field, (void) v;
	return false;
}

static bool dm_nil_fieldget(dm_value self, const char *field, dm_value *v) {
	(void) self, (void) field, (void) v;
	return false;
}

dm_module dm_nil_init(dm_state *dm) {
	(void) dm;
	dm_module m = {0};
	m.compare = dm_nil_compare;
	m.fieldset_s = dm_nil_fieldset;
	m.fieldget_s = dm_nil_fieldget;
	return m;
}