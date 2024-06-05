#include <stdio.h>
#include <string.h>
#include <dm_table.h>

#define TABLE_INVALID_VAL ((dm_value){DM_TYPE_NIL, {.int_val = 0xDEAD}})
#define TABLE_INVALID_CODE (0xDEAD)

struct dm_table {
	dm_gc_obj gc_header;
	int size;
	void *keys;
	void *values;
	struct dm_table *parent;
};

static bool table_is_invalid_entry(dm_value v) {
	return v.type == DM_TYPE_NIL && v.int_val == TABLE_INVALID_CODE;
}

static void table_mark(dm_state *dm, struct dm_gc_obj *obj) {
	dm_table *t = (dm_table*) obj;
	dm_value *keys = (dm_value*) t->keys;
	dm_value *values = (dm_value*) t->values;
	for (int i = 0; i < t->size; i++) {
		if (table_is_invalid_entry(keys[i]) || table_is_invalid_entry(values[i])) {
			continue;
		}
		if (dm_value_is_gc_obj(keys[i])) {
			dm_gc_mark(dm, keys[i].gc_obj);
		}
		if (dm_value_is_gc_obj(values[i])) {
			dm_gc_mark(dm, values[i].gc_obj);
		}
	}
}

static void table_free(dm_state *dm, struct dm_gc_obj *obj) {
	dm_table *t = (dm_table*) obj;
	dm_value *keys = (dm_value*) t->keys;
	dm_value *values = (dm_value*) t->values;
	for (int i = 0; i < t->size; i++) {
		if (table_is_invalid_entry(keys[i]) || table_is_invalid_entry(values[i])) {
			continue;
		}
		if (dm_value_is_gc_obj(keys[i])) {
			dm_gc_free(dm, keys[i].gc_obj);
		}
		if (dm_value_is_gc_obj(values[i])) {
			dm_gc_free(dm, values[i].gc_obj);
		}
	}
	free(t->keys);
	free(t->values);
}

dm_value dm_value_table(dm_state *dm, int size) {
	dm_table *table = (dm_table*) dm_gc_malloc(dm, sizeof(dm_table), table_mark, table_free);
	table->size = size < 16 ? 16 : size;
	int bytes = sizeof(dm_value) * table->size;
	table->keys = malloc(bytes);
	table->values = malloc(bytes);
	table->parent = NULL;

	dm_value *keys = (dm_value*) table->keys;
	dm_value *values = (dm_value*) table->values;
	for (int i = 0; i < table->size; i++) {
		keys[i] = TABLE_INVALID_VAL;
		values[i] = TABLE_INVALID_VAL;
	}
	return (dm_value){DM_TYPE_TABLE, {.table_val = table}};
}

bool dm_table_equal(dm_table *t1, dm_table *t2) {
	return t1 == t2;
}

static void dm_table_inspect(dm_state *dm, dm_value self) {
	dm_table *t = self.table_val;
	dm_value *keys = t->keys;
	dm_value *values = t->values;

	printf("{");

	int printed = 0;
	for (int i = 0; i < t->size; i++) {
		dm_value key = keys[i];
		dm_value value = values[i];
		if (key.int_val == TABLE_INVALID_CODE || value.int_val == TABLE_INVALID_CODE) {
			continue;
		}

		dm_value_inspect(dm, key);
		printf(": ");
		dm_value_inspect(dm, value);
		printf(", ");
		printed++;
	}

	if (printed != 0) {
		printf("\b\b");
	}

	printf("}");
}

void dm_value_table_set(dm_state *dm, dm_value t, dm_value field, dm_value v) {
	if (t.type != DM_TYPE_TABLE) {
		return;
	}

	dm_table *table = t.table_val;
	dm_value *keys = (dm_value*) table->keys;
	dm_value *values = (dm_value*) table->values;
	for (int i = 0; i < table->size; i++) {
		if (dm_value_equals(dm, keys[i], field)) {
			values[i] = v;
			break;
		}
		if (keys[i].int_val != TABLE_INVALID_CODE && values[i].int_val != TABLE_INVALID_CODE) {
			continue;
		}
		keys[i] = field;
		values[i] = v;
		break;
	}
}

dm_value dm_value_table_get(dm_state *dm, dm_value t, dm_value field) {
	if (t.type != DM_TYPE_TABLE) {
		// error
		return dm_value_nil();
	}

	dm_table *table = t.table_val;
	dm_value *keys = (dm_value*) table->keys;
	dm_value *values = (dm_value*) table->values;
	for (int i = 0; i < table->size; i++) {
		if (keys[i].int_val == TABLE_INVALID_CODE || values[i].int_val == TABLE_INVALID_CODE) {
			continue;
		}
		if (dm_value_equals(dm, keys[i], field)) {
			return values[i];
		}
	}

	return dm_value_nil();
}

static bool dm_table_equals(dm_state *dm, dm_value self, dm_value other) {
	(void) dm;
	return self.table_val == other.table_val ? true : false;
}

static bool dm_table_fieldset(dm_state *dm, dm_value self, const char *field, dm_value v) {
	(void) dm, (void) self, (void) field, (void) v;
	return false;
}

static bool dm_table_fieldget(dm_state *dm, dm_value self, const char *field, dm_value *v) {
	(void) dm, (void) self, (void) field, (void) v;
	return false;
}

dm_module dm_table_init(dm_state *dm) {
	(void) dm;
	dm_module m = dm_module_default(dm);
	m.typename = "table";
	m.inspect = dm_table_inspect;
	m.equals = dm_table_equals;
	m.fieldset_s = dm_table_fieldset;
	m.fieldget_s = dm_table_fieldget;
	return m;
}
