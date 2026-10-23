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

	do
	{
		pid = waitpid(-j->pgid, &status, WUNTRACED | WCONTINUED);
		if (pid > 0)
		{
			_tl_jobs_mark_process_status(pid, status);
		}
		else
		{
			if (errno == ECHILD)
			{
				// No more children in this process group
				break;
			}
		}
	} while (!tl_jobs_is_stopped(j) && !tl_jobs_is_completed(j));

	// If the job has completed, update its state
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

void tl_jobs_to_foreground(job_t *j, int cont)
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
	}

	_tl_jobs_wait(j);

	/* Reclaim terminal control back for the shell */
	if (termlink_job_control_enabled)
	{
		pid_t shell_pgid = getpid(); // or use a global shell_pgid captured at startup
		tcsetpgrp(STDIN_FILENO, shell_pgid);
	}

	LOG("tl_jobs_to_foreground: done job pgid=%d", (int)j->pgid);

	tl_jobs_cleanup();
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

void tl_jobs_launch_process(process_t *p, pid_t infd, pid_t outfd, pid_t errfd, int foreground)
{
	LOG("tl_jobs_launch_process: start process command='%s', infd=%d, outfd=%d, errfd=%d, foreground=%d",
		p->command ? p->command : "(null)", (int)infd, (int)outfd, (int)errfd, foreground);

	pid_t pid = getpid();

	// Assign pid directly to the process structure
	if (p)
	{
		p->pid = pid;
	}

	/* Put the process into its process group */
	if (setpgid(pid, pid) < 0)
	{
		LOG("tl_jobs_launch_process: setpgid failed (errno=%d)", errno);
	}

	/* If this is a foreground process and job control is active, give it terminal control */
	if (foreground && termlink_job_control_enabled)
	{
		tcsetpgrp(STDIN_FILENO, pid);
	}

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
		LOG("tl_jobs_launch_process: redirecting stdin from fd %d", (int)infd);
		dup2(infd, STDIN_FILENO);
		close(infd);
	}

	/* Handle standard output redirection */
	if (outfd != STDOUT_FILENO)
	{
		LOG("tl_jobs_launch_process: redirecting stdout to fd %d", (int)outfd);
		dup2(outfd, STDOUT_FILENO);
		close(outfd);
	}

	/* Handle standard error redirection */
	if (errfd != STDERR_FILENO)
	{
		LOG("tl_jobs_launch_process: redirecting stderr to fd %d", (int)errfd);
		dup2(errfd, STDERR_FILENO);
		close(errfd);
	}

	LOG("tl_jobs_launch_process: done for process pid=%d", (int)pid);
}

void tl_jobs_launch_job(job_t *j, int foreground)
{
	LOG("tl_jobs_launch_job: start job name='%s', foreground=%d", j->name ? j->name : "(null)", foreground);

	// Add job to global job list
	j->next = g_first_job;
	g_first_job = j;

	// For single or multiple processes in the job, iterate and fork/launch
	process_t *p = j->first_process;
	pid_t pid = fork();

	/* Inside child fork branch / tl_jobs_launch_process */
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGTSTP, SIG_DFL);

	if (pid < 0)
	{
		LOG("tl_jobs_launch_job: fork failed");
		return;
	}

	if (pid == 0)
	{
		/* Child process */
		pid_t child_pid = getpid();

		// If pgid is not set yet, use the first child's pid as the pgid
		if (j->pgid == 0)
		{
			j->pgid = child_pid;
		}
		setpgid(child_pid, j->pgid);

		// Call your existing fully-instrumented launcher
		tl_jobs_launch_process(p, STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO, foreground);

		// Exec handling happens right after in the execution routine,
		// or we can structure the child exit here.
	}
	else
	{
		/* Parent process */
		p->pid = pid;
		if (j->pgid == 0)
		{
			j->pgid = pid;
		}
		setpgid(pid, j->pgid);

		if (foreground)
		{
			tl_jobs_to_foreground(j, 0);
		}
		else
		{
			tl_jobs_to_background(j, 0);
			LOG("tl_jobs_launch_job: background job launched, pgid=%d", (int)j->pgid);
		}
	}

	LOG("tl_jobs_launch_job: done");
}

void tl_jobs_cleanup(void)
{
	job_t **curr = &g_first_job;
	while (*curr)
	{
		job_t *j = *curr;
		if (j->state == JOB_DONE)
		{
			LOG("tl_jobs_cleanup: removing completed job pgid=%d", (int)j->pgid);
			*curr = j->next; // unlink

			// Free processes
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