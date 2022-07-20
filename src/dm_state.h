#pragma once

#include <dm_value.h>

typedef struct {
	dm_value main;
} dm_state;

dm_state *dm_open(void);
void dm_close(dm_state *dm);
