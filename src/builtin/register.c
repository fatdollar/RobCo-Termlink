#include "main.h"
#include "builtin.h"

/* ============================================================
 * GLOBAL STATE
 * ============================================================ */

builtin_entry builtins[MAX_BUILTINS];
int builtin_count = 0;

/* ============================================================
 *  COMMAND
 * ============================================================ */

void builtin_register(const char *name, builtin_func func)
{
    builtins[builtin_count].name = name;
    builtins[builtin_count].func = func;
    builtin_count++;
}

int builtin_try(int argc, char **argv)
{
    for (int i = 0; i < builtin_count; i++)
    {
        if (strcmp(argv[0], builtins[i].name) == 0)
            return builtins[i].func(argc, argv);
    }
    return 0;
}