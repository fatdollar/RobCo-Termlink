#include "main.h"
#include "jobs.h"

static job *g_first_job = NULL;

void init_job_control(void)
{
    /* Ensure shell is running interactively */
    pid_t shell_pgid = getpid();
    if (setpgid(shell_pgid, shell_pgid) < 0)
    {
        /* Fallback if setpgid fails in non-tty environments */
        return;
    }
    tcsetpgrp(STDIN_FILENO, shell_pgid);
}

void put_job_in_foreground(job *j, int cont)
{
    /* Put job in the foreground */
    tcsetpgrp(STDIN_FILENO, j->pgid);

    /* Send continue signal if stopped */
    if (cont)
    {
        kill(-j->pgid, SIGCONT);
        j->state = JOB_RUNNING;
    }

    /* Wait for it to finish or stop */
    // TODO: implement wait_for_job(j) loop here

    /* Reclaim terminal control back to shell */
    tcsetpgrp(STDIN_FILENO, getpgrp());
}

void put_job_in_background(job *j, int cont)
{
    if (cont)
    {
        kill(-j->pgid, SIGCONT);
        j->state = JOB_RUNNING;
    }
}