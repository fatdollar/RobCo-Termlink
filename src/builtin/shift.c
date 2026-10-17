#include "main.h"
#include "builtin.h"
#include "terminal.h"

/* positional parameters from set.c */
extern char *positional_params[256];
extern int positional_count;

int builtin_shift(int argc, char **argv)
{
    LOG("shift: argc=%d", argc);

    int n = 1; /* default shift amount */

    if (argc > 1)
    {
        n = atoi(argv[1]);
        if (n < 0)
        {
            terminal_slow_print("shift: invalid shift count\n");
            return 1;
        }
    }

    LOG("shift: shifting by %d", n);

    if (n == 0 || positional_count == 0)
        return 1;

    if (n >= positional_count)
    {
        /* remove all parameters */
        LOG("shift: removing all positional parameters");

        for (int i = 0; i < positional_count; i++)
            free(positional_params[i]);

        positional_count = 0;
        return 1;
    }

    /* free first n parameters */
    for (int i = 0; i < n; i++)
    {
        LOG("shift: freeing param[%d]='%s'", i, positional_params[i]);
        free(positional_params[i]);
    }

    /* shift remaining parameters left */
    for (int i = n; i < positional_count; i++)
    {
        positional_params[i - n] = positional_params[i];
        LOG("shift: param[%d] now '%s'", i - n, positional_params[i]);
    }

    positional_count -= n;

    return 1;
}