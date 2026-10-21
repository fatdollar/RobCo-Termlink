#include "builtin.h"
#include "terminal.h"

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_echo(int argc, char **argv)
{
    int i = 1;
    int newline = 1;

    LOG("echo: argc=%d", argc);

    if (i < argc && strcmp(argv[i], "-n") == 0)
    {
        newline = 0;
        i++;
        LOG("echo: -n detected, suppressing newline");
    }

    for (; i < argc; i++)
    {
        terminal_slow_print(argv[i]);
        if (i + 1 < argc)
            terminal_slow_print(" ");
    }

    if (newline)
        terminal_slow_print("\n");

    return 1;
}
