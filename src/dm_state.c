#include <stdlib.h>
#include <stdio.h>
#include <dm_state.h>
#include <dm.h>
#include <dm_gc.h>
#include <setjmp.h>

struct dm_state {
	dm_gc gc;
	dm_module modules[DM_TYPE_NUM_TYPES];
	dm_value main;
	jmp_buf error_jump_buf;
	bool debug;
	bool runtime_error;
};

static int required_field_null(dm_module *m, const char *field) {
	printf("[dm] Required field '%s' of module '%s' is not set\n", field, m->typename);
	return 1;
}

static int init_modules(dm_state *dm) {
	dm->modules[DM_TYPE_NIL] = dm_nil_init(dm);
	dm->modules[DM_TYPE_BOOL] = dm_bool_init(dm);
	dm->modules[DM_TYPE_INT] = dm_int_init(dm);
	dm->modules[DM_TYPE_FLOAT] = dm_float_init(dm);
	dm->modules[DM_TYPE_STRING] = dm_string_init(dm);
	dm->modules[DM_TYPE_ARRAY] = dm_array_init(dm);
	dm->modules[DM_TYPE_TABLE] = dm_table_init(dm);
	dm->modules[DM_TYPE_FUNCTION] = dm_function_init(dm);

	int error = 0;
	for (int i = 0; i < DM_TYPE_NUM_TYPES; i++) {
		if (dm->modules[i].typename == NULL) {
			printf("Module at index %d has no name\n", i);
			error = 1;
			continue;
		}
		if (dm->modules[i].inspect == NULL) {
			error = required_field_null(&dm->modules[i], "inspect");
		}
		if (dm->modules[i].equals == NULL) {
			error = required_field_null(&dm->modules[i], "equals");
		}
		if (dm->modules[i].fieldget_s == NULL) {
			error = required_field_null(&dm->modules[i], "fieldget_s");
		}
		if (dm->modules[i].fieldset_s == NULL) {
			error = required_field_null(&dm->modules[i], "fieldset_s");
		}
	}

	return error;
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
	if (init_modules(dm) != 0) {
		free(dm);
		return NULL;
	}

	dm->main = dm_value_nil();
	dm->debug = false;
	dm->runtime_error = false;
	return dm;
}

dm_value *dm_state_get_main(dm_state *dm) {
	return &dm->main;
}

void dm_enable_debug(dm_state *dm) {
	dm->debug = true;
}

bool dm_debug_enabled(dm_state *dm) {
	return dm->debug;
}

void *dm_state_get_gc(dm_state *dm) {
	return (void*) &dm->gc;
}

dm_module *dm_state_get_module(dm_state *dm, dm_type t) {
	return &dm->modules[t];
}

jmp_buf *dm_state_get_jmpbuf(dm_state *dm) {
	return &dm->error_jump_buf;
}

dm_exception void dm_state_set_error(dm_state *dm, const char *message, va_list args) {
	dm->runtime_error = true;

	printf("RuntimeError: ");
	vprintf(message, args);
	printf("\n");
	va_end(args);

	longjmp(dm->error_jump_buf, 1);
}

void dm_state_reset_error(dm_state *dm) {
	dm->runtime_error = false;
}

bool dm_state_has_error(dm_state *dm) {
	return dm->runtime_error;
}

void dm_close(dm_state *dm) {
	dm_gc_deinit(dm);
	free(dm);
}
