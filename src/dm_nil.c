#include <stdio.h>
#include <dm_nil.h>

static int dm_nil_compare(dm_value self, dm_value other) {
	(void) self;
	(void) other;
	return 0;
}

dm_module dm_nil_init(dm_state *dm) {
	(void) dm;
	dm_module m = {0};
	m.compare = dm_nil_compare;
	return m;
}
