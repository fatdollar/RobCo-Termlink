#ifndef JOBS_H
#define JOBS_H

#include "main.h"

typedef enum
{
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_DONE
} job_state;

typedef struct process
{
    pid_t pid;
    char *command;
    struct process *next;
    bool completed;
    bool stopped;
    int status;
} process;

typedef struct job
{
    char *name;
    process *first_process;
    pid_t pgid;
    bool notified;
    job_state state;
    struct job *next;
} job;

void init_job_control(void);
void launch_process(process *p, pid_t infd, pid_t outfd, pid_t errfd, int foreground);
void launch_job(job *j, int foreground);
void put_job_in_foreground(job *j, int cont);
void put_job_in_background(job *j, int cont);
job *find_job(pid_t pgid);

#endif