#pragma once

#include <dm.h>
#include <dm_value.h>

dm_state *dm_open(void);
void *dm_state_get_main(dm_state *dm);
void dm_state_set_main(dm_state *dm, dm_value main);
void *dm_state_get_gc(dm_state *dm);
dm_module *dm_state_get_module(dm_state *dm, dm_type t);
void dm_state_set_error(dm_state *dm, const char *message);
bool dm_state_has_error(dm_state *dm);
void dm_close(dm_state *dm);

