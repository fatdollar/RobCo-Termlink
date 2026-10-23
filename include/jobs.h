#ifndef JOBS_H
#define JOBS_H

#include "main.h"

typedef enum
{
	JOB_RUNNING,
	JOB_STOPPED,
	JOB_DONE
} job_state;

typedef struct process_t
{
	pid_t pid;
	char *command;
	struct process_t *next;
	bool completed;
	bool stopped;
	int status;
} process_t;

typedef struct job_t
{
	char *name;
	process_t *first_process;
	pid_t pgid;
	bool notified;
	job_state state;
	struct job_t *next;
} job_t;

extern job_t *g_first_job;

void tl_jobs_init(void);
job_t *tl_jobs_find(pid_t pgid);
void tl_jobs_to_foreground(job_t *j, int cont);
void tl_jobs_to_background(job_t *j, int cont);
void tl_jobs_launch_process(process_t *p, pid_t infd, pid_t outfd, pid_t errfd, int foreground);
void tl_jobs_launch_job(job_t *j, int foreground);
void tl_jobs_cleanup(void);

#endif