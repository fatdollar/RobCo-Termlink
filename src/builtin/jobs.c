#include "main.h"
#include "builtin.h"
#include "terminal.h"
#include "jobs.h"

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_jobs(int argc, char **argv)
{
	LOG("builtin_jobs: start");
	tl_jobs_cleanup(); // Clean up finished jobs first

	if (!g_first_job)
	{
		return 0; // POSIX: no output or clean exit if no jobs exist
	}

	bool opt_l = false;
	bool opt_p = false;
	int first_operand_idx = 1;

	/* Simple POSIX option parsing for -l and -p */
	for (int i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-' && argv[i][1] != '\0')
		{
			if (strcmp(argv[i], "--") == 0)
			{
				first_operand_idx = i + 1;
				break;
			}
			for (int j = 1; argv[i][j] != '\0'; j++)
			{
				if (argv[i][j] == 'l')
					opt_l = true;
				else if (argv[i][j] == 'p')
					opt_p = true;
				else
				{
					fprintf(stderr, "jobs: illegal option -- '%c'\n", argv[i][j]);
					return 2; // POSIX utility syntax error exit code
				}
			}
			first_operand_idx = i + 1;
		}
		else
		{
			break;
		}
	}

	int job_num = 1;
	for (job_t *j = g_first_job; j; j = j->next, job_num++)
	{
		// Determine state string per POSIX spec
		const char *state_str = "Running";
		if (j->state == JOB_STOPPED)
			state_str = "Stopped";
		else if (j->state == JOB_DONE)
			state_str = "Done";

		// Determine current job marker (+ for last/current, - for previous, space otherwise)
		// (Assuming g_first_job or a tracker points to current/previous jobs)
		char marker = ' ';
		if (j == g_first_job) // Simplified current job marker assignment
			marker = '+';

		char buf[512];

		if (opt_p)
		{
			// -p: Display only the process group ID / PID
			snprintf(buf, sizeof(buf), "%d\n", j->pgid);
			terminal_slow_print(buf);
		}
		else if (opt_l)
		{
			// -l: Include process group ID before state
			snprintf(buf, sizeof(buf), "[%d]  %c %d %s %s\n",
					 job_num, marker, j->pgid, state_str, j->name ? j->name : "");
			terminal_slow_print(buf);
		}
		else
		{
			// Default POSIX format: [%d] %c %s %s
			snprintf(buf, sizeof(buf), "[%d]  %c %s %s\n",
					 job_num, marker, state_str, j->name ? j->name : "");
			terminal_slow_print(buf);
		}
	}

	fflush(stdout);
	LOG("builtin_jobs: done");
	return 0;
}