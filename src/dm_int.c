#include <stdio.h>
#include <dm_int.h>
#include <dm_state.h>
#include <dm.h>

static void dm_int_inspect(dm_state *dm, dm_value self) {
	(void) dm;
	printf("%ld", self.int_val);
}

static bool dm_int_equals(dm_state *dm, dm_value self, dm_value other) {
	if (other.type == DM_TYPE_FLOAT) {
		return dm_value_equals(dm, other, self);
	} else if (other.type == DM_TYPE_INT) {
		return self.int_val == other.int_val;
	}

	return false;
}

static bool dm_int_fieldset(dm_state *dm, dm_value self, const char *field, dm_value v) {
	(void) dm, (void) self, (void) field, (void) v;
	return false;
}

static bool dm_int_fieldget(dm_state *dm, dm_value self, const char *field, dm_value *v) {
	(void) dm, (void) self, (void) field, (void) v;
	return false;
}

dm_value dm_int_negate(dm_state *dm, dm_value self) {
	(void) dm;
	return dm_value_int(-self.int_val);
}

static int dm_int_compare(dm_state *dm, dm_value self, dm_value other) {
	if (other.type == DM_TYPE_FLOAT) {
		dm_module *m = dm_state_get_module(dm, other.type);
		return -1 * m->compare(dm, other, self);
	} else if (other.type == DM_TYPE_INT) {
		dm_int a = self.int_val;
		dm_int b = other.int_val;
		return a < b ? -1 : a == b ? 0 : 1;
	}

	dm_runtime_compare_mismatch(dm, self, other);
}

static dm_value dm_int_add(dm_state *dm, dm_value self, dm_value other) {
	if (other.type == DM_TYPE_INT) {
		return dm_value_int(self.int_val + other.int_val);
	} else if (other.type == DM_TYPE_FLOAT) {
		return dm_value_float((dm_float) self.int_val + other.float_val);
	}

	dm_runtime_type_mismatch2(dm, DM_TYPE_INT, DM_TYPE_FLOAT, other);
}

static dm_value dm_int_sub(dm_state *dm, dm_value self, dm_value other) {
	if (other.type == DM_TYPE_INT) {
		return dm_value_int(self.int_val - other.int_val);
	} else if (other.type == DM_TYPE_FLOAT) {
		return dm_value_float((dm_float) self.int_val - other.float_val);
	}

	dm_runtime_type_mismatch2(dm, DM_TYPE_INT, DM_TYPE_FLOAT, other);
}

static dm_value dm_int_mul(dm_state *dm, dm_value self, dm_value other) {
	if (other.type == DM_TYPE_INT) {
		return dm_value_int(self.int_val * other.int_val);
	} else if (other.type == DM_TYPE_FLOAT) {
		return dm_value_float((dm_float) self.int_val * other.float_val);
	}

	dm_runtime_type_mismatch2(dm, DM_TYPE_INT, DM_TYPE_FLOAT, other);
}

static dm_value dm_int_div(dm_state *dm, dm_value self, dm_value other) {
	if (other.type != DM_TYPE_INT && other.type != DM_TYPE_FLOAT) {
		dm_runtime_type_mismatch2(dm, DM_TYPE_INT, DM_TYPE_FLOAT, other);
	}

	if (other.type == DM_TYPE_INT) {
		if (other.int_val == 0) {
			dm_runtime_error(dm, "division by 0");
		}

		return dm_value_int(self.int_val / other.int_val);
	} else {
		if (other.float_val == 0) {
			dm_runtime_error(dm, "division by 0");
		}

		return dm_value_float((dm_float) self.int_val / other.float_val);
	}
}

static dm_value dm_int_mod(dm_state *dm, dm_value self, dm_value other) {
	if (other.type != DM_TYPE_INT) {
		dm_runtime_type_mismatch(dm, DM_TYPE_INT, other);
	}

	if (other.int_val == 0) {
		dm_runtime_error(dm, "division by 0");
	}

	return dm_value_int(self.int_val % other.int_val);
}

dm_module dm_int_init(dm_state *dm) {
	(void) dm;
	dm_module m = dm_module_default(dm);
	m.typename = "integer";
	m.inspect = dm_int_inspect;
	m.equals = dm_int_equals;
	m.fieldset_s = dm_int_fieldset;
	m.fieldget_s = dm_int_fieldget;
	m.compare = dm_int_compare;
	m.add = dm_int_add;
	m.sub = dm_int_sub;
	m.mul = dm_int_mul;
	m.div = dm_int_div;
	m.mod = dm_int_mod;
	return m;
}
