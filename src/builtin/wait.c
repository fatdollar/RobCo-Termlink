#include "main.h"
#include "builtin.h"
#include "terminal.h"
#include "jobs.h"

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_wait(int argc, char **argv)
{
	LOG("wait: argc=%d", argc);

	int last_status = 0;

	/* Case 1: No arguments -> wait for all active background children/jobs */
	if (argc < 2)
	{
		LOG("wait: waiting for all background jobs");
		pid_t pid;
		int status;
		while ((pid = wait(&status)) > 0)
		{
			LOG("wait: reaped child pid=%d", (int)pid);
			if (WIFEXITED(status))
				last_status = WEXITSTATUS(status);
			else if (WIFSIGNALED(status))
				last_status = 128 + WTERMSIG(status);
		}
		return 0;
	}

	/* Case 2: Wait for specific PIDs or job IDs (e.g., wait 1234 or wait %1) */
	for (int i = 1; i < argc; i++)
	{
		const char *arg = argv[i];
		pid_t target_pid = 0;

		if (arg[0] == '%')
		{
			arg++;
			int target_job_num = atoi(arg);
			job_t *j = g_first_job;
			int current_idx = 1;
			while (j)
			{
				if (current_idx == target_job_num)
				{
					target_pid = j->pgid;
					break;
				}
				j = j->next;
				current_idx++;
			}
		}
		else
		{
			target_pid = (pid_t)atoi(arg);
		}

		if (target_pid <= 0)
		{
			terminal_slow_print("wait: invalid PID or job spec: ");
			terminal_slow_print(argv[i]);
			terminal_slow_print("\n");
			last_status = 127;
			continue;
		}

		LOG("wait: waiting for pid=%d", (int)target_pid);
		int status = 0;
		if (waitpid(target_pid, &status, 0) < 0)
		{
			if (errno != ECHILD)
			{
				LOG("waitpid error: %s", strerror(errno));
			}
			last_status = 127; // POSIX: return 127 if specified ID is unknown/not a child
		}
		else
		{
			if (WIFEXITED(status))
				last_status = WEXITSTATUS(status);
			else if (WIFSIGNALED(status))
				last_status = 128 + WTERMSIG(status);
		}
	}

	return last_status;
}