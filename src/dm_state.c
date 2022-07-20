#include <stdlib.h>

#include <dm_state.h>


dm_state *dm_open(void) {
	dm_state *dm = malloc(sizeof(dm_state));
	dm->main = dm_value_nil();
	return dm;
}

void dm_close(dm_state *dm) {
	free(dm);
}
