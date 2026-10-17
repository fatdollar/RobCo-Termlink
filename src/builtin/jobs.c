#include "main.h"
#include "builtin.h"
#include "terminal.h"

int builtin_jobs(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    LOG("jobs: stub called");
    terminal_slow_print("jobs: job control not implemented\n");

    return 1;
}