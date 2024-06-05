#pragma once

#include <dm_value.h>

struct dm_function {
	dm_gc_obj gc_header;
	void *chunk;
	int nargs;
	bool takes_self;
};

dm_module dm_function_init(dm_state *dm);
