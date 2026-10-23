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
		const char *arg = argv[1];
		if (arg[0] == '%')
			arg++; // skip optional leading '%'

		int target_idx = atoi(arg);
		int current_idx = 1;

		j = NULL;
		for (job_t *curr = g_first_job; curr; curr = curr->next, current_idx++)
		{
			if (current_idx == target_idx || curr->pgid == (pid_t)target_idx)
			{
				j = curr;
				break;
			}
		}
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