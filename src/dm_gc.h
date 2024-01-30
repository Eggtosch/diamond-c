#pragma once

#include <stdlib.h>
#include <stdbool.h>

typedef struct dm_state dm_state;
typedef struct dm_gc dm_gc;
struct dm_gc_obj;

typedef void (*dm_gc_mark_fn)(dm_state*, struct dm_gc_obj*);
typedef void (*dm_gc_free_fn)(dm_state*, struct dm_gc_obj*);

typedef struct dm_gc_obj {
	struct dm_gc_obj *next;
	struct dm_gc_obj *previous;
	dm_gc_mark_fn mark;
	dm_gc_free_fn free;
	int marked;
} dm_gc_obj;

typedef struct dm_gc {
	dm_gc_obj *first;
	dm_gc_obj *last;
	bool in_deinit;
} dm_gc;

void dm_gc_init(dm_state *dm);
void dm_gc_deinit(dm_state *dm);
dm_gc_obj *dm_gc_malloc(dm_state *dm, size_t size, dm_gc_mark_fn mark, dm_gc_free_fn free);
void dm_gc_free(dm_state *dm, dm_gc_obj *obj);
void dm_gc_collect(dm_state *dm, dm_gc_obj *root);
void dm_gc_mark(dm_state *dm, dm_gc_obj *obj);
