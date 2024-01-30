#include <stdlib.h>
#include <stdio.h>
#include <dm_state.h>
#include <dm.h>
#include <dm_gc.h>

struct dm_state {
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
	dm->modules[DM_TYPE_ARRAY] = dm_array_init(dm);
	dm->modules[DM_TYPE_TABLE] = (dm_module){0};
	dm->modules[DM_TYPE_FUNCTION] = (dm_module){0};
}

char *dm_read_file(const char *path) {
	FILE *file = fopen(path, "rb");
	if (file == NULL) {
		fprintf(stderr, "Could not open file \"%s\".\n", path);
		exit(1);
	}

	fseek(file, 0L, SEEK_END);
	size_t file_size = ftell(file);
	rewind(file);

	char *buffer = malloc(file_size + 1);

	size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
	if (bytes_read < file_size) {
		fprintf(stderr, "Could not read whole file \"%s\".\n", path);
		exit(1);
	}

	buffer[bytes_read] = '\0';

	fclose(file);
	return buffer;
}

dm_state *dm_open(void) {
	dm_state *dm = malloc(sizeof(dm_state));
	dm_gc_init(dm);
	init_modules(dm);
	dm->runtime_error = false;
	return dm;
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
