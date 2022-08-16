#include <stdlib.h>
#include <dm_state.h>
#include <dm_value.h>
#include <dm_gc.h>

struct dm_state {
	dm_value main;
	dm_gc gc;
};

dm_state *dm_open(void) {
	dm_state *dm = malloc(sizeof(dm_state));
	dm->main = dm_value_nil();
	dm_gc_init(dm);
	return dm;
}

void *dm_state_get_main(dm_state *dm) {
	return (void*) &dm->main;
}

void dm_state_set_main(dm_state *dm, dm_value main) {
	dm->main = main;
}

void *dm_state_get_gc(dm_state *dm) {
	return (void*) &dm->gc;
}

void dm_close(dm_state *dm) {
	dm_gc_deinit(dm);
	free(dm);
}

