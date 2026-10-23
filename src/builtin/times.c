#include "main.h"
#include "builtin.h"
#include "terminal.h"

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_times(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	LOG("times: called");

	struct tms t;
	times(&t);

	char buf[128];
	snprintf(buf, sizeof(buf), "%ldm%lds %ldm%lds\n%ldm%lds %ldm%lds\n",
			 (long)(t.tms_utime / 60 / CLOCKS_PER_SEC), (long)((t.tms_utime / CLOCKS_PER_SEC) % 60),
			 (long)(t.tms_stime / 60 / CLOCKS_PER_SEC), (long)((t.tms_stime / CLOCKS_PER_SEC) % 60),
			 (long)(t.tms_cutime / 60 / CLOCKS_PER_SEC), (long)((t.tms_cutime / CLOCKS_PER_SEC) % 60),
			 (long)(t.tms_cstime / 60 / CLOCKS_PER_SEC), (long)((t.tms_cstime / CLOCKS_PER_SEC) % 60));

	terminal_slow_print(buf);
	return 0;
}