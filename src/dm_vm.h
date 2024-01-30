#pragma once

#include <dm_value.h>
#include <dm_state.h>

dm_value dm_vm_exec(dm_state *dm, char *prog);
