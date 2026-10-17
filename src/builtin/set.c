#include "main.h"
#include "builtin.h"
#include "terminal.h"

/* POSIX positional parameters storage */
char *positional_params[256];
int positional_count = 0;

int builtin_set(int argc, char **argv)
{
    LOG("set: argc=%d", argc);

    /* Case 1: no arguments → print environment variables */
    if (argc == 1)
    {
        LOG("set: printing environment");

        extern char **environ;
        for (char **env = environ; *env != NULL; env++)
        {
            terminal_slow_print(*env);
            terminal_slow_print("\n");
        }
        return 1;
    }

    /* Case 2: set -- args... → positional parameters */
    if (strcmp(argv[1], "--") == 0)
    {
        LOG("set: setting positional parameters");

        /* Clear old positional parameters */
        for (int i = 0; i < positional_count; i++)
            free(positional_params[i]);

        positional_count = argc - 2;

        for (int i = 0; i < positional_count; i++)
        {
            positional_params[i] = strdup(argv[i + 2]);
            LOG("set: param[%d]='%s'", i, positional_params[i]);
        }

        return 1;
    }

    /* Case 3: stub for -o / +o options */
    if (argv[1][0] == '-' || argv[1][0] == '+')
    {
        terminal_slow_print("set: option handling not implemented yet\n");
        return 1;
    }

    /* Fallback */
    terminal_slow_print("set: usage: set [-- args...]\n");
    return 1;
}