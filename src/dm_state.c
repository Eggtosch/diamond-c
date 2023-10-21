#include <stdlib.h>
#include <stdio.h>
#include <dm_state.h>
#include <dm_value.h>
#include <dm_gc.h>
#include <dm_nil.h>
#include <dm_bool.h>
#include <dm_int.h>
#include <dm_float.h>
#include <dm_string.h>

struct dm_state {
	dm_value main;
	dm_gc gc;
	dm_module modules[DM_TYPE_NUM_TYPES];
	bool runtime_error;
};

static void init_modules(dm_state *dm) {
	dm->modules[DM_TYPE_NIL] = dm_nil_init(dm);
	dm->modules[DM_TYPE_BOOL] = dm_bool_init(dm);
	dm->modules[DM_TYPE_INT] = dm_int_init(dm);
	dm->modules[DM_TYPE_FLOAT] = dm_float_init(dm);
	dm->modules[DM_TYPE_STRING] = dm_string_init(dm);
	dm->modules[DM_TYPE_ARRAY] = (dm_module){0};
	dm->modules[DM_TYPE_TABLE] = (dm_module){0};
	dm->modules[DM_TYPE_FUNCTION] = (dm_module){0};
}

dm_state *dm_open(void) {
	dm_state *dm = malloc(sizeof(dm_state));
	dm->main = dm_value_nil();
	dm_gc_init(dm);
	init_modules(dm);
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

dm_module *dm_state_get_module(dm_state *dm, dm_type t) {
	return &dm->modules[t];
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
