#include "main.h"
#include "builtin.h"
#include "terminal.h"

int builtin_wait(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    LOG("wait: stub called");
    terminal_slow_print("wait: job control not implemented\n");

    return 1;
}