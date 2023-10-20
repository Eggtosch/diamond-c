#include <stdio.h>
#include <dm_bool.h>

static int dm_bool_compare(dm_value self, dm_value other) {
	return self.bool_val - other.bool_val;
}

dm_module dm_bool_init(dm_state *dm) {
	(void) dm;
	dm_module m = {0};
	m.compare = dm_bool_compare;
	return m;
}
