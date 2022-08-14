#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dm_generic_array.h>

#define DM_ARR_MIN_CAPACITY (16)

static int default_compare(int size, void *e1, void *e2) {
	return memcmp(e1, e2, size);
}

void dm_gen_array_new(dm_gen_array *arr, int typesize, int (*compare)(int size, void *e1, void *e2)) {
	*arr = (dm_gen_array) {
		.typesize = typesize,
		.capacity = 0,
		.size     = 0,
		.compare  = compare == NULL ? default_compare : compare,
		.data     = NULL
	};
}

void dm_gen_array_free(dm_gen_array *arr) {
	free(arr->data);
	*arr = (dm_gen_array) {
		.typesize = 0,
		.capacity = 0,
		.size     = 0,
		.compare  = NULL,
		.data     = NULL
	};
}

void dm_gen_array_clear(dm_gen_array *arr) {
	if (arr->data == NULL) {
		return;
	}
	memset(arr->data, 0, arr->size * arr->typesize);
	arr->size = 0;
}

int dm_gen_array_size(dm_gen_array *arr) {
	return arr->size;
}

int dm_gen_array_push(dm_gen_array *arr, void *item) {
	if (arr->size >= arr->capacity) {
		if (arr->capacity < DM_ARR_MIN_CAPACITY) {
			arr->capacity = 16;
		} else {
			arr->capacity *= 2;
		}
		arr->data = realloc(arr->data, arr->capacity * arr->typesize);
	}
	memcpy(arr->data + arr->size * arr->typesize, item, arr->typesize);
	return arr->size++;
}

bool dm_gen_array_pop(dm_gen_array *arr, void *item) {
	if (arr->size <= 0) {
		return false;
	}
	arr->size--;
	memcpy(item, arr->data + arr->size * arr->typesize, arr->typesize);
	return true;
}

int dm_gen_array_index(dm_gen_array *arr, void *item) {
	void *current = arr->data;
	for (int i = 0; i < arr->size; i++) {
		if (arr->compare(arr->typesize, current, item) == 0) {
			return i;
		}
		current += arr->typesize;
	}
	return -1;
}

void *dm_gen_array_get(dm_gen_array *arr, int index) {
	if (index < 0 || index >= arr->size) {
		return NULL;
	}
	return arr->data + index * arr->typesize;
}

