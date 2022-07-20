#pragma once

#include <dm_state.h>

typedef enum {
	DM_VM_OK = 0,
	DM_VM_COMPILE_ERROR = 1,
	DM_VM_RUNTIME_ERROR = 2
} dm_vm_status;

#define DM_VM_OK            (0)
#define DM_VM_COMPILE_ERROR (1)
#define DM_VM_RUNTIME_ERROR (2)

int dm_vm_exec(dm_state *dm, char *prog);
