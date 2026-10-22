#include "main.h"
#include "builtin.h"
#include "terminal.h"
#include "jobs.h"

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_bg(int argc, char **argv)
{
	LOG("builtin_bg: start");

	job_t *j = g_first_job;
	if (argc > 1)
	{
		pid_t pgid = (pid_t)atoi(argv[1]);
		j = tl_jobs_find(pgid);
	}

	if (!j)
	{
		terminal_slow_print("bg: no such job\n");
		return 1;
	}

	tl_jobs_to_background(j, 1);

	char buf[256];
	snprintf(buf, sizeof(buf), "[%d] running %s\n", (int)j->pgid, j->name ? j->name : "");
	terminal_slow_print(buf);

	LOG("builtin_bg: done");
	return 0;
}