#pragma once

#include <dm_value.h>
#include <dm_state.h>

int dm_vm_exec(dm_state *dm, char *prog, dm_value *result, bool repl);
