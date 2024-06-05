#pragma once

#include <dm_value.h>

dm_module dm_string_init(dm_state *dm);
size_t dm_string_size(dm_string *s);
const char *dm_string_c_str(dm_string *s);
