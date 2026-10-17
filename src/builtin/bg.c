#include "main.h"
#include "builtin.h"
#include "terminal.h"

int builtin_bg(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    LOG("bg: stub called");
    terminal_slow_print("bg: job control not implemented\n");

    return 1;
}
