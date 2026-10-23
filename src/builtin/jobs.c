#include "main.h"
#include "builtin.h"
#include "terminal.h"
#include "jobs.h"

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_jobs(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	LOG("builtin_jobs: start");
	tl_jobs_cleanup(); // Clean up finished jobs first

	if (!g_first_job)
	{
		terminal_slow_print("No jobs.");
		LOG("builtin_jobs: no jobs found");
		return 0;
	}

	int job_num = 1;
	for (job_t *j = g_first_job; j; j = j->next, job_num++)
	{
		const char *state_str = "Running";
		if (j->state == JOB_STOPPED)
			state_str = "Stopped";
		else if (j->state == JOB_DONE)
			state_str = "Done";

		char buf[512];
		snprintf(buf, sizeof(buf), "[%d]  %s %s\n", job_num, state_str, j->name ? j->name : "");
		terminal_slow_print(buf);
	}

	LOG("builtin_jobs: done");
	return 0;
}