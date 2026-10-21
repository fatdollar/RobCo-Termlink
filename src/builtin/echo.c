#include "builtin.h"
#include "terminal.h"

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_echo(int argc, char **argv)
{
    int i = 1;
    int suppress_newline = 0;

    LOG("echo: argc=%d", argc);

    for (int j = 0; j < argc; j++)
        LOG("echo: argv[%d]='%s'", j, argv[j]);

    if (argc > 1 && strcmp(argv[1], "-n") == 0)
    {
        suppress_newline = 1;
        i = 2;

        LOG("echo: -n detected");
    }

    for (; i < argc; i++)
    {
        LOG("echo: printing '%s'", argv[i]);
        terminal_slow_print(argv[i]);

        if (!suppress_newline)
        {
            LOG("echo: printing newline");
            terminal_slow_print("\n");
        }
        else
        {
            LOG("echo: newline suppressed");
        }
    }

    LOG("echo: done");

    return 0;
}