#include "debug.h"

#ifdef DEBUG
FILE *debug_log = NULL;

void debug_init(void)
{
	debug_log = fopen("termlink_debug.log", "a");
	if (!debug_log)
	{
		// fallback: no logging
	}
}
#endif