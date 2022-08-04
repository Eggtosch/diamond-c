#pragma once

#include <stdbool.h>

typedef struct {
	int typesize;
	int capacity;
	int size;
	int (*compare)(int size, void *e1, void *e2);
	void *data;
} dm_array;

void dm_array_new(dm_array *arr, int typesize, int (*compare)(int size, void *e1, void *e2));
void dm_array_free(dm_array *arr);
void dm_array_clear(dm_array *arr);
int dm_array_size(dm_array *arr);
int dm_array_push(dm_array *arr, void *item);
bool dm_array_pop(dm_array *arr, void *item);
int dm_array_index(dm_array *arr, void *item);
void *dm_array_get(dm_array *arr, int index);

