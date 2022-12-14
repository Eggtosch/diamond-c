#include <stdlib.h>
#include <stdio.h>
#include <dm_state.h>
#include <dm_value.h>
#include <dm_gc.h>

struct dm_state {
	dm_value main;
	dm_gc gc;
	bool runtime_error;
};

dm_state *dm_open(void) {
	dm_state *dm = malloc(sizeof(dm_state));
	dm->main = dm_value_nil();
	dm_gc_init(dm);
	dm->runtime_error = false;
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

void dm_state_set_error(dm_state *dm, const char *message) {
	if (message) {
		printf("RuntimeError: %s\n", message);
	}
	dm->runtime_error = message != NULL;
}

bool dm_state_has_error(dm_state *dm) {
	return dm->runtime_error;
}

void dm_close(dm_state *dm) {
	dm_gc_deinit(dm);
	free(dm);
}

