#include "main.h"
#include "terminal.h"
#include "jobs.h"

/* ============================================================
 *  STATE
 * ============================================================ */

job_t *g_first_job = NULL;

/* ============================================================
 * FORWARD DECLARATIONS
 * ============================================================ */

void _tl_jobs_wait(job_t *j);
void _tl_jobs_mark_process_status(pid_t pid, int status);
static int tl_jobs_is_stopped(job_t *j);
static int tl_jobs_is_completed(job_t *j);

/* ============================================================
 * INTERNAL HELPERS
 * ============================================================ */

void _tl_jobs_wait(job_t *j)
{
	LOG("_tl_jobs_wait: start waiting for job pgid=%d", (int)j->pgid);
	int status;
	pid_t pid;

	while (!tl_jobs_is_stopped(j) && !tl_jobs_is_completed(j))
	{
		pid = waitpid(-j->pgid, &status, WUNTRACED | WCONTINUED);
		if (pid > 0)
		{
			_tl_jobs_mark_process_status(pid, status);
		}
		else
		{
			if (errno == ECHILD || errno == EPERM)
			{
				// No more children or invalid reference; prevent infinite hang
				break;
			}
			if (errno == EINTR)
			{
				continue;
			}
			// Other unexpected error, break to avoid infinite loop
			break;
		}
	}

	// If all processes are done, mark job completed
	if (tl_jobs_is_completed(j))
	{
		j->state = JOB_DONE;
		LOG("_tl_jobs_wait: job pgid=%d completed", (int)j->pgid);
	}

	LOG("_tl_jobs_wait: finished waiting for job pgid=%d", (int)j->pgid);
}

void _tl_jobs_mark_process_status(pid_t pid, int status)
{
	LOG("_tl_jobs_mark_process_status: pid=%d, status=%d", (int)pid, status);
	for (job_t *j = g_first_job; j; j = j->next)
	{
		for (process_t *p = j->first_process; p; p = p->next)
		{
			if (p->pid == pid)
			{
				p->status = status;
				if (WIFSTOPPED(status))
				{
					p->stopped = true;
					p->completed = false;
					j->state = JOB_STOPPED;
					LOG("_tl_jobs_mark_process_status: process pid=%d stopped by signal %d", (int)pid, WSTOPSIG(status));
				}
				else if (WIFCONTINUED(status))
				{
					p->stopped = false;
					p->completed = false;
					j->state = JOB_RUNNING;
					LOG("_tl_jobs_mark_process_status: process pid=%d continued", (int)pid);
				}
				else
				{
					p->completed = true;
					p->stopped = false;
					if (WIFEXITED(status))
					{
						LOG("_tl_jobs_mark_process_status: process pid=%d exited with status %d", (int)pid, WEXITSTATUS(status));
					}
					else if (WIFSIGNALED(status))
					{
						LOG("_tl_jobs_mark_process_status: process pid=%d terminated by signal %d", (int)pid, WTERMSIG(status));
					}
				}
				return;
			}
		}
	}
	LOG("_tl_jobs_mark_process_status: warning - no process matched pid=%d", (int)pid);
}

static int tl_jobs_is_stopped(job_t *j)
{
	for (process_t *p = j->first_process; p; p = p->next)
	{
		if (!p->completed && !p->stopped)
			return 0;
	}
	return 1;
}

static int tl_jobs_is_completed(job_t *j)
{
	for (process_t *p = j->first_process; p; p = p->next)
	{
		if (!p->completed)
			return 0;
	}
	return 1;
}

/* ============================================================
 * SETUP
 * ============================================================ */

void tl_jobs_init(void)
{
	LOG("tl_jobs_init: start");
	pid_t shell_pgid = getpid();
	if (setpgid(shell_pgid, shell_pgid) < 0)
	{
		LOG("tl_jobs_init: setpgid failed (errno=%d)", errno);
		return;
	}
	tcsetpgrp(STDIN_FILENO, shell_pgid);
	LOG("tl_jobs_init: done, shell_pgid=%d", (int)shell_pgid);
}

/* ============================================================
 * JOB MANAGEMENT
 * ============================================================ */

job_t *tl_jobs_find(pid_t pgid)
{
	LOG("tl_jobs_find: searching for pgid=%d", (int)pgid);
	for (job_t *j = g_first_job; j; j = j->next)
	{
		if (j->pgid == pgid)
		{
			LOG("tl_jobs_find: found job name='%s'", j->name ? j->name : "(null)");
			return j;
		}
	}
	LOG("tl_jobs_find: job not found for pgid=%d", (int)pgid);
	return NULL;
}

int tl_jobs_to_foreground(job_t *j, int cont)
{
	LOG("tl_jobs_to_foreground: start job pgid=%d, cont=%d", (int)j->pgid, cont);

	if (termlink_job_control_enabled)
	{
		tcsetpgrp(STDIN_FILENO, j->pgid);
	}

	if (cont)
	{
		LOG("tl_jobs_to_foreground: sending SIGCONT to pgid=%d", (int)j->pgid);
		kill(-j->pgid, SIGCONT);
		j->state = JOB_RUNNING;

		for (process_t *p = j->first_process; p; p = p->next)
		{
			p->stopped = false;
		}
	}

	_tl_jobs_wait(j);

	/* Reclaim terminal control back for the shell */
	if (termlink_job_control_enabled)
	{
		pid_t shell_pgid = getpid();
		tcsetpgrp(STDIN_FILENO, shell_pgid);
	}

	int last_status = 0;
	process_t *p = j->first_process;
	while (p)
	{
		if (p->stopped || j->state == JOB_STOPPED)
		{
			last_status = 128 + (WIFSTOPPED(p->status) ? WSTOPSIG(p->status) : SIGTSTP);
			char buf[512];
			snprintf(buf, sizeof(buf), "\n[1]+  Stopped                     %s\n", j->name ? j->name : "");
			terminal_slow_print(buf);
			LOG("tl_jobs_to_foreground: job stopped, status=%d", last_status);
			return last_status; // Job is stopped, so do NOT clean up yet.
		}
		else if (WIFSIGNALED(p->status))
		{
			int sig = WTERMSIG(p->status);
			last_status = 128 + sig;
			char buf[512];
			if (sig == SIGINT)
			{
				snprintf(buf, sizeof(buf), "\n[1]+  Terminated                  %s\n", j->name ? j->name : "");
				terminal_slow_print(buf);
			}
			else
			{
				snprintf(buf, sizeof(buf), "\n[1]+  Signal %d                    %s\n", sig, j->name ? j->name : "");
				terminal_slow_print(buf);
			}
			break;
		}
		else if (WIFEXITED(p->status))
		{
			last_status = WEXITSTATUS(p->status);
		}
		p = p->next;
	}

	LOG("tl_jobs_to_foreground: done job pgid=%d, status=%d", (int)j->pgid, last_status);

	/* Now safe to clean up the completed job */
	tl_jobs_cleanup();

	return last_status;
}

void tl_jobs_to_background(job_t *j, int cont)
{
	LOG("tl_jobs_to_background: start job pgid=%d, cont=%d", (int)j->pgid, cont);
	if (cont)
	{
		LOG("tl_jobs_to_background: sending SIGCONT to pgid=%d", (int)j->pgid);
		kill(-j->pgid, SIGCONT);
		j->state = JOB_RUNNING;
	}
	LOG("tl_jobs_to_background: done job pgid=%d", (int)j->pgid);
}
void tl_jobs_launch_process(process_t *p, pid_t infd, pid_t outfd, pid_t errfd, int foreground, pid_t pgid)
{
	LOG("tl_jobs_launch_process: start process command='%s', pgid=%d, foreground=%d",
		p->command ? p->command : "(null)", (int)pgid, foreground);

	pid_t pid = getpid();
	if (p)
	{
		p->pid = pid;
	}

	/* Put the process into the shared job/pipeline process group */
	if (setpgid(pid, pgid) < 0)
	{
		LOG("tl_jobs_launch_process: setpgid failed (errno=%d)", errno);
	}

	/* If foreground, the parent handles tcsetpgrp; avoid child-side races */
	(void)foreground;

	/* Restore default signal handlers for the child process */
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGTSTP, SIG_DFL);
	signal(SIGTTIN, SIG_DFL);
	signal(SIGTTOU, SIG_DFL);
	signal(SIGCHLD, SIG_DFL);

	/* Handle standard input redirection */
	if (infd != STDIN_FILENO)
	{
		dup2(infd, STDIN_FILENO);
		close(infd);
	}

	/* Handle standard output redirection */
	if (outfd != STDOUT_FILENO)
	{
		dup2(outfd, STDOUT_FILENO);
		close(outfd);
	}

	/* Handle standard error redirection */
	if (errfd != STDERR_FILENO)
	{
		dup2(errfd, STDERR_FILENO);
		close(errfd);
	}

	LOG("tl_jobs_launch_process: done for process pid=%d", (int)pid);
}

void tl_jobs_cleanup(void)
{
	pid_t pid;
	int status;

	/* Step 1: Non-blocking reap of any finished child processes */
	while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0)
	{
		_tl_jobs_mark_process_status(pid, status);
	}

	/* Step 2: Mark running jobs as DONE if all processes inside them have completed */
	for (job_t *j = g_first_job; j; j = j->next)
	{
		if (j->state == JOB_RUNNING && tl_jobs_is_completed(j))
		{
			j->state = JOB_DONE;
			LOG("tl_jobs_cleanup: job pgid=%d marked as DONE", (int)j->pgid);
		}
	}

	/* Step 3: Unlink and free fully completed jobs from the global list */
	job_t **curr = &g_first_job;
	while (*curr)
	{
		job_t *j = *curr;
		if (j->state == JOB_DONE)
		{
			LOG("tl_jobs_cleanup: removing completed job pgid=%d", (int)j->pgid);
			*curr = j->next; // unlink

			/* Free processes */
			process_t *p = j->first_process;
			while (p)
			{
				process_t *next_p = p->next;
				free(p->command);
				free(p);
				p = next_p;
			}
			free(j->name);
			free(j);
		}
		else
		{
			curr = &(j->next);
		}
	}
}