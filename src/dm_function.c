#include <stdio.h>
#include <dm_function.h>
#include <dm_chunk.h>

static void function_free(dm_state *dm, struct dm_gc_obj *obj) {
	(void) dm;
	dm_function *func = (dm_function*) obj;
	dm_chunk_free(func->chunk);
	free(func->chunk);
}

dm_value dm_value_function(dm_state *dm, void *chunk, int nargs, bool takes_self) {
	dm_function *func = (dm_function*) dm_gc_malloc(dm, sizeof(dm_function), NULL, function_free);
	func->chunk = chunk;
	func->nargs = nargs;
	func->takes_self = takes_self;
	return (dm_value){DM_TYPE_FUNCTION, {.func_val = func}};
}

static void dm_function_inspect(dm_state *dm, dm_value self) {
	(void) dm;
	printf("<function %p>", self.func_val);
}

static bool dm_function_equals(dm_state *dm, dm_value self, dm_value other) {
	(void) dm;
	return other.type == DM_TYPE_FUNCTION && self.func_val == other.func_val;
}

static bool dm_function_fieldset(dm_state *dm, dm_value self, const char *field, dm_value v) {
	(void) dm, (void) self, (void) field, (void) v;
	return false;
}

static bool dm_function_fieldget(dm_state *dm, dm_value self, const char *field, dm_value *v) {
	(void) dm, (void) self, (void) field, (void) v;
	return false;
}

dm_module dm_function_init(dm_state *dm) {
	(void) dm;
	dm_module m = dm_module_default(dm);
	m.typename = "function";
	m.equals = dm_function_equals;
	m.inspect = dm_function_inspect;
	m.fieldset_s = dm_function_fieldset;
	m.fieldget_s = dm_function_fieldget;
	return m;
}
