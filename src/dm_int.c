#include <stdio.h>
#include <dm_int.h>

static int dm_int_compare(dm_value self, dm_value other) {
	dm_int a = self.int_val;
	dm_int b = other.int_val;
	return a < b ? -1 : a == b ? 0 : 1;
}

static bool dm_int_fieldget(dm_value self, dm_value field, dm_value *v) {
	(void) self, (void) field, (void) v;
	return false;
}

static bool dm_int_fieldset(dm_value self, dm_value field, dm_value v) {
	(void) self, (void) field, (void) v;
	return false;
}

dm_value dm_int_negate(dm_value self) {
	return dm_value_int(-self.int_val);
}

static dm_value dm_int_add(dm_value self, dm_value other) {
	if (other.type == DM_TYPE_INT) {
		return dm_value_int(self.int_val + other.int_val);
	} else if (other.type == DM_TYPE_FLOAT) {
		return dm_value_float((dm_float) self.int_val + other.float_val);
	}
	return dm_value_nil();
}

static dm_value dm_int_sub(dm_value self, dm_value other) {
	if (other.type == DM_TYPE_INT) {
		return dm_value_int(self.int_val - other.int_val);
	} else if (other.type == DM_TYPE_FLOAT) {
		return dm_value_float((dm_float) self.int_val - other.float_val);
	}
	return dm_value_nil();
}

static dm_value dm_int_mul(dm_value self, dm_value other) {
	if (other.type == DM_TYPE_INT) {
		return dm_value_int(self.int_val * other.int_val);
	} else if (other.type == DM_TYPE_FLOAT) {
		return dm_value_float((dm_float) self.int_val * other.float_val);
	}
	return dm_value_nil();
}

static dm_value dm_int_div(dm_value self, dm_value other) {
	if (other.type == DM_TYPE_INT) {
		return dm_value_int(self.int_val / other.int_val);
	} else if (other.type == DM_TYPE_FLOAT) {
		return dm_value_float((dm_float) self.int_val / other.float_val);
	}
	return dm_value_nil();
}

static dm_value dm_int_mod(dm_value self, dm_value other) {
	if (other.type == DM_TYPE_INT) {
		return dm_value_int(self.int_val % other.int_val);
	}
	return dm_value_nil();
}

dm_module dm_int_init(dm_state *dm) {
	(void) dm;
	dm_module m = {0};
	m.compare = dm_int_compare;
	m.fieldget = dm_int_fieldget;
	m.fieldset = dm_int_fieldset;
	m.add = dm_int_add;
	m.sub = dm_int_sub;
	m.mul = dm_int_mul;
	m.div = dm_int_div;
	m.mod = dm_int_mod;
	return m;
}
