#include <stdio.h>
#include <dm_float.h>

static int dm_float_compare(dm_value self, dm_value other) {
	dm_float a = self.float_val;
	dm_float b = other.float_val;
	return a < b ? -1 : a == b ? 0: 1;
}

static void dm_float_inspect(dm_value self) {
	printf("%g", self.float_val);
}

dm_value dm_float_negate(dm_value self) {
	return dm_value_float(-self.float_val);
}

static dm_value dm_float_add(dm_value self, dm_value other) {
	if (other.type == DM_TYPE_FLOAT) {
		return dm_value_float(self.float_val + other.float_val);
	} else if (other.type == DM_TYPE_INT) {
		return dm_value_float(self.float_val + (dm_float) other.int_val);
	}
	return dm_value_nil();
}

static dm_value dm_float_sub(dm_value self, dm_value other) {
	if (other.type == DM_TYPE_FLOAT) {
		return dm_value_float(self.float_val - other.float_val);
	} else if (other.type == DM_TYPE_INT) {
		return dm_value_float(self.float_val - (dm_float) other.int_val);
	}
	return dm_value_nil();
}

static dm_value dm_float_mul(dm_value self, dm_value other) {
	if (other.type == DM_TYPE_FLOAT) {
		return dm_value_float(self.float_val * other.float_val);
	} else if (other.type == DM_TYPE_INT) {
		return dm_value_float(self.float_val * (dm_float) other.int_val);
	}
	return dm_value_nil();
}

static dm_value dm_float_div(dm_value self, dm_value other) {
	if (other.type == DM_TYPE_FLOAT) {
		return dm_value_float(self.float_val / other.float_val);
	} else if (other.type == DM_TYPE_INT) {
		return dm_value_float(self.float_val / (dm_float) other.int_val);
	}
	return dm_value_nil();
}

dm_module dm_float_init(dm_state *dm) {
	(void) dm;
	dm_module m;
	m.compare = dm_float_compare;
	m.inspect = dm_float_inspect;
	m.add = dm_float_add;
	m.sub = dm_float_sub;
	m.mul = dm_float_mul;
	m.div = dm_float_div;
	m.mod = NULL;
	return m;
}
