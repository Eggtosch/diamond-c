#pragma once

#include <dm_value.h>

char *dm_read_file(const char *path);
dm_state *dm_open(void);
dm_value *dm_state_get_main(dm_state *dm);
void dm_enable_debug(dm_state *dm);
bool dm_debug_enabled(dm_state *dm);
void *dm_state_get_gc(dm_state *dm);
dm_module *dm_state_get_module(dm_state *dm, dm_type t);
void dm_state_set_error(dm_state *dm, const char *message);
bool dm_state_has_error(dm_state *dm);
void dm_close(dm_state *dm);
