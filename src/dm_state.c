#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include <dm_state.h>
#include <dm.h>
#include <dm_gc.h>

struct string_constant {
	const char *data;
	int size;
};

struct dm_state {
	dm_gc gc;
	dm_module modules[DM_TYPE_NUM_TYPES];
	dm_value main;
	jmp_buf error_jump_buf;

	int strings_size;
	int strings_capacity;
	struct string_constant *strings;

	bool debug;
	bool runtime_error;
};

char *dm_read_file(const char *path) {
	FILE *file = fopen(path, "rb");
	if (file == NULL) {
		fprintf(stderr, "Could not open file \"%s\".\n", path);
		exit(1);
	}

	fseek(file, 0, SEEK_END);
	size_t file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

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

dm_state *dm_open(void) {
	dm_state *dm = calloc(1, sizeof(dm_state));
	dm_gc_init(dm);
	if (init_modules(dm) != 0) {
		free(dm);
		return NULL;
	}

	dm->main = dm_value_nil();
	return dm;
}

void dm_close(dm_state *dm) {
	dm_gc_deinit(dm);
	free(dm->strings);
	free(dm);
}

const char *dm_state_string_dedup(dm_state *dm, const char *str, int str_len) {
	for (int i = 0; i < dm->strings_size; i++) {
		struct string_constant *sc = &dm->strings[i];
		if (str_len == sc->size && strncmp(str, sc->data, sc->size) == 0) {
			return sc->data;
		}
	}

	char *new_data = malloc(str_len + 1);
	memcpy(new_data, str, str_len);
	new_data[str_len] = '\0';

	if (dm->strings_size >= dm->strings_capacity) {
		if (dm->strings_capacity < 16) {
			dm->strings_capacity = 16;
		} else {
			dm->strings_capacity *= 2;
		}

		dm->strings = realloc(dm->strings, sizeof(struct string_constant) * dm->strings_capacity);
	}

	dm->strings[dm->strings_size++] = (struct string_constant){.data = new_data, .size = str_len};
	return new_data;
}

const char *dm_state_get_string_data(dm_state *dm, int i) {
	if (i < 0 || i >= dm->strings_size) {
		dm_runtime_error(dm, "interned string constant %d out of bounds %d", i, dm->strings_size);
	}

	return dm->strings[i].data;
}

int dm_state_get_string_size(dm_state *dm, int i) {
	if (i < 0 || i >= dm->strings_size) {
		dm_runtime_error(dm, "interned string constant %d out of bounds %d", i, dm->strings_size);
	}

	return dm->strings[i].size;
}

void dm_enable_debug(dm_state *dm) {
	dm->debug = true;
}

bool dm_debug_enabled(dm_state *dm) {
	return dm->debug;
}

dm_value *dm_state_get_main(dm_state *dm) {
	return &dm->main;
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
