#include <dm.h>
#include <dm_state.h>

dm_value dm_value_nil(void) {
	return (dm_value){DM_TYPE_NIL, {0}};
}

dm_value dm_value_bool(dm_bool bool_val) {
	return (dm_value){DM_TYPE_BOOL, {.bool_val = bool_val}};
}

dm_value dm_value_int(dm_int int_val) {
	return (dm_value){DM_TYPE_INT, {.int_val = int_val}};
}

dm_value dm_value_float(dm_float float_val) {
	return (dm_value){DM_TYPE_FLOAT, {.float_val = float_val}};
}

bool dm_value_equals(dm_state *dm, dm_value v1, dm_value v2) {
	dm_module *m = dm_state_get_module(dm, v1.type);
	return m->equals(dm, v1, v2);
}

void dm_value_inspect(dm_state *dm, dm_value v) {
	dm_module *m = dm_state_get_module(dm, v.type);
	return m->inspect(dm, v);
}

bool dm_value_is_gc_obj(dm_value v) {
	return v.type == DM_TYPE_STRING
		|| v.type == DM_TYPE_ARRAY
		|| v.type == DM_TYPE_TABLE
		|| v.type == DM_TYPE_FUNCTION;
}

const char *dm_value_type_str(dm_state *dm, dm_value v) {
	dm_module *m = dm_state_get_module(dm, v.type);
	return m->typename;
}

static int compare_fallback(dm_state *dm, dm_value self, dm_value other) {
	dm_runtime_compare_mismatch(dm, self, other);
}

static dm_value add_fallback(dm_state *dm, dm_value self, dm_value other) {
	(void) other;
	dm_runtime_no_method_error(dm, "+", self);
}

static dm_value sub_fallback(dm_state *dm, dm_value self, dm_value other) {
	(void) other;
	dm_runtime_no_method_error(dm, "-", self);
}

static dm_value mul_fallback(dm_state *dm, dm_value self, dm_value other) {
	(void) other;
	dm_runtime_no_method_error(dm, "*", self);
}

static dm_value div_fallback(dm_state *dm, dm_value self, dm_value other) {
	(void) other;
	dm_runtime_no_method_error(dm, "/", self);
}

static dm_value mod_fallback(dm_state *dm, dm_value self, dm_value other) {
	(void) other;
	dm_runtime_no_method_error(dm, "%", self);
}

dm_module dm_module_default(dm_state *dm) {
	(void) dm;

	dm_module m = {0};
	m.compare = compare_fallback;
	m.add = add_fallback;
	m.sub = sub_fallback;
	m.mul = mul_fallback;
	m.div = div_fallback;
	m.mod = mod_fallback;
	return m;
}
