#include <stdio.h>
#include <dm_float.h>

static int dm_float_compare(dm_state *dm, dm_value self, dm_value other) {
	(void) dm;
	dm_float a = self.float_val;
	dm_float b = other.float_val;
	return a < b ? -1 : a == b ? 0: 1;
}

static bool dm_float_fieldset(dm_state *dm, dm_value self, const char *field, dm_value v) {
	(void) dm, (void) self, (void) field, (void) v;
	return false;
}

static bool dm_float_fieldget(dm_state *dm, dm_value self, const char *field, dm_value *v) {
	(void) dm, (void) self, (void) field, (void) v;
	return false;
}

dm_value dm_float_negate(dm_state *dm, dm_value self) {
	(void) dm;
	return dm_value_float(-self.float_val);
}

static dm_value dm_float_add(dm_state *dm, dm_value self, dm_value other) {
	(void) dm;
	if (other.type == DM_TYPE_FLOAT) {
		return dm_value_float(self.float_val + other.float_val);
	} else if (other.type == DM_TYPE_INT) {
		return dm_value_float(self.float_val + (dm_float) other.int_val);
	}
	return dm_value_nil();
}

static dm_value dm_float_sub(dm_state *dm, dm_value self, dm_value other) {
	(void) dm;
	if (other.type == DM_TYPE_FLOAT) {
		return dm_value_float(self.float_val - other.float_val);
	} else if (other.type == DM_TYPE_INT) {
		return dm_value_float(self.float_val - (dm_float) other.int_val);
	}
	return dm_value_nil();
}

static dm_value dm_float_mul(dm_state *dm, dm_value self, dm_value other) {
	(void) dm;
	if (other.type == DM_TYPE_FLOAT) {
		return dm_value_float(self.float_val * other.float_val);
	} else if (other.type == DM_TYPE_INT) {
		return dm_value_float(self.float_val * (dm_float) other.int_val);
	}
	return dm_value_nil();
}

static dm_value dm_float_div(dm_state *dm, dm_value self, dm_value other) {
	(void) dm;
	if (other.type == DM_TYPE_FLOAT) {
		return dm_value_float(self.float_val / other.float_val);
	} else if (other.type == DM_TYPE_INT) {
		return dm_value_float(self.float_val / (dm_float) other.int_val);
	}
	return dm_value_nil();
}

dm_module dm_float_init(dm_state *dm) {
	(void) dm;
	dm_module m = {0};
	m.compare = dm_float_compare;
	m.fieldset_s = dm_float_fieldset;
	m.fieldget_s = dm_float_fieldget;
	m.add = dm_float_add;
	m.sub = dm_float_sub;
	m.mul = dm_float_mul;
	m.div = dm_float_div;
	return m;
}
