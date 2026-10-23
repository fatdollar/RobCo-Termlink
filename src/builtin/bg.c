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
	int target_job_num = -1;
	pid_t target_pgid = -1;

	// If an operand is provided, parse it (defaulting to the current/last job if omitted)
	if (argc > 1)
	{
		const char *arg = argv[1];
		if (arg[0] == '%')
			arg++; // skip optional leading '%'

		// Check if it's purely numeric
		target_job_num = atoi(arg);
		target_pgid = (pid_t)target_job_num;
	}

	j = NULL;
	int current_idx = 1;
	int actual_job_num = 1;

	for (job_t *curr = g_first_job; curr; curr = curr->next, current_idx++)
	{
		if (argc > 1)
		{
			// Match either by job index (e.g., %1) or by process group ID
			if (current_idx == target_job_num || curr->pgid == target_pgid)
			{
				j = curr;
				actual_job_num = current_idx;
				break;
			}
		}
		else
		{
			// POSIX: If no operand is given, the current job is selected
			// (Assuming g_first_job or a designated pointer represents the current job)
			j = curr;
			actual_job_num = current_idx;
			break; // Selects the primary/most recent job by default
		}
	}

	if (!j)
	{
		fprintf(stderr, "bg: no such job\n");
		return 1; // POSIX requires non-zero exit status on failure
	}

	// Send SIGCONT to place the job in the background
	tl_jobs_to_background(j, 1);

	// POSIX output format: [%d] command &
	char buf[256];
	snprintf(buf, sizeof(buf), "[%d] %s &\n", actual_job_num, j->name ? j->name : "");
	terminal_slow_print(buf);
	fflush(stdout);

	LOG("builtin_bg: done");
	return 0; // Success
}