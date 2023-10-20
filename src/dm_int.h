#pragma once

#include <dm_value.h>
#include <dm.h>

dm_module dm_int_init(dm_state *dm);
dm_value dm_int_negate(dm_value self);
