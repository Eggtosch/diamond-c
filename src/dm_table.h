#pragma once

#include <dm_value.h>

dm_module dm_table_init(dm_state *dm);
void dm_value_table_set(dm_state *dm, dm_value t, dm_value field, dm_value v);
dm_value dm_value_table_get(dm_state *dm, dm_value t, dm_value field);
