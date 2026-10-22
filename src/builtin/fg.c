#include "main.h"
#include "builtin.h"
#include "terminal.h"
#include "jobs.h"

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_fg(int argc, char **argv)
{
	LOG("builtin_fg: start");

	job_t *j = g_first_job;
	if (argc > 1)
	{
		pid_t pgid = (pid_t)atoi(argv[1]);
		j = tl_jobs_find(pgid);
	}

	if (!j)
	{
		terminal_slow_print("fg: no such job\n");
		return 1;
	}

	char buf[256];
	snprintf(buf, sizeof(buf), "%s\n", j->name ? j->name : "");
	terminal_slow_print(buf);

	tl_jobs_to_foreground(j, 1);

	LOG("builtin_fg: done");
	return 0;
}