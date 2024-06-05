#pragma once

#include <dm_value.h>

#include <dm_nil.h>
#include <dm_bool.h>
#include <dm_int.h>
#include <dm_float.h>
#include <dm_string.h>
#include <dm_array.h>
#include <dm_table.h>
#include <dm_function.h>

dm_value dm_runtime_error(dm_state *dm, const char *message, ...);
int dm_runtime_compare_mismatch(dm_state *dm, dm_value v1, dm_value v2);
dm_value dm_runtime_no_method_error(dm_state *dm, const char *method, dm_value v);
