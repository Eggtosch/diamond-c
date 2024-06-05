#pragma once

#include <dm_value.h>

dm_module dm_array_init(dm_state *dm);
void dm_value_array_set(dm_value a, dm_value index, dm_value v);
dm_value dm_value_array_get(dm_value a, dm_value index);
