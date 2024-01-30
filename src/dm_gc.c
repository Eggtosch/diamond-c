#include <stdio.h>
#include <dm_gc.h>
#include <dm_state.h>

void dm_gc_init(dm_state *dm) {
	dm_gc *gc = dm_state_get_gc(dm);
	gc->first = NULL;
	gc->last = NULL;
	gc->in_deinit = false;
}

void dm_gc_deinit(dm_state *dm) {
	dm_gc *gc = dm_state_get_gc(dm);
	gc->in_deinit = true;
	for (dm_gc_obj *obj = gc->first; obj != NULL;) {
		dm_gc_obj *next_obj = obj->next;
		if (obj->free != NULL) {
			obj->free(dm, obj);
		}
		free(obj);
		obj = next_obj;
	}
	gc->first = NULL;
	gc->last = NULL;
}

dm_gc_obj *dm_gc_malloc(
	dm_state *dm,
	size_t size,
	void (*mark_fn)(dm_state *dm, dm_gc_obj *obj),
	void (*free_fn)(dm_state *dm, dm_gc_obj *obj)
) {
	dm_gc *gc = dm_state_get_gc(dm);
	dm_gc_obj *ptr = malloc(size);
	ptr->previous = NULL;
	ptr->next = gc->first;
	gc->first = ptr;
	if (ptr->next != NULL) {
		ptr->next->previous = ptr;
	}
	ptr->mark = mark_fn;
	ptr->free = free_fn;
	ptr->marked = 0;
	return ptr;
}

void dm_gc_free(dm_state *dm, dm_gc_obj *obj) {
	dm_gc *gc = dm_state_get_gc(dm);
	if (gc->in_deinit) {
		return;
	}
	if (obj->previous == NULL) {
		gc->first = obj->next;
	} else {
		obj->previous->next = obj->next;
	}
	if (obj->next == NULL) {
		gc->last = obj->previous;
	} else {
		obj->next->previous = obj->previous;
	}

	if (obj->free != NULL) {
		obj->free(dm, obj);
	}
	free(obj);
}

void dm_gc_collect(dm_state *dm, dm_gc_obj *root) {
	dm_gc *gc = dm_state_get_gc(dm);
	dm_gc_mark(dm, root);
	for (dm_gc_obj *obj = gc->first; obj != NULL;) {
		dm_gc_obj *next_obj = obj->next;
		if (obj->marked == 0) {
			dm_gc_free(dm, obj);
		} else {
			obj->marked = 0;
		}
		obj = next_obj;
	}
}

void dm_gc_mark(dm_state *dm, dm_gc_obj *obj) {
	if (obj->marked == 1) {
		return;
	}
	obj->marked = 1;
	if (obj->mark != NULL) {
		obj->mark(dm, obj);
	}
}
