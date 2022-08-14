#pragma once

#include <stdbool.h>

typedef struct {
	int typesize;
	int capacity;
	int size;
	int (*compare)(int size, void *e1, void *e2);
	void *data;
} dm_gen_array;

void  dm_gen_array_new(dm_gen_array *arr, int typesize, int (*compare)(int size, void *e1, void *e2));
void  dm_gen_array_free(dm_gen_array *arr);
void  dm_gen_array_clear(dm_gen_array *arr);
int   dm_gen_array_size(dm_gen_array *arr);
int   dm_gen_array_push(dm_gen_array *arr, void *item);
bool  dm_gen_array_pop(dm_gen_array *arr, void *item);
int   dm_gen_array_index(dm_gen_array *arr, void *item);
void *dm_gen_array_get(dm_gen_array *arr, int index);

