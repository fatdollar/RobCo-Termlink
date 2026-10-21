#include "main.h"
#include "builtin.h"
#include "terminal.h"

#define MAX_ALIASES 256

/* ============================================================
 * GLOBAL STATE
 * ============================================================ */

char *alias_names[MAX_ALIASES];
char *alias_values[MAX_ALIASES];
int alias_count = 0;

/* ============================================================
 *  FORWARD DECLARATIONS
 * ============================================================ */

static int _builtin_alias_find(const char *name);
static void _builtin_alias_print(int idx);

/* ============================================================
 *  INTERNAL HELPERS
 * ============================================================ */

static int _builtin_alias_find(const char *name)
{
    for (int i = 0; i < alias_count; i++)
    {
        if (strcmp(alias_names[i], name) == 0)
            return i;
    }
    return -1;
}

static void _builtin_alias_print(int idx)
{
    terminal_slow_print(alias_names[idx]);
    terminal_slow_print("='");
    terminal_slow_print(alias_values[idx]);
    terminal_slow_print("'\n");
}

/* ============================================================
 *  COMMAND
 * ============================================================ */

 int builtin_alias(int argc, char **argv)
{
    LOG("alias: argc=%d", argc);

    /* Case 1: no args → print all aliases */
    if (argc == 1)
    {
        for (int i = 0; i < alias_count; i++)
            _builtin_alias_print(i);
        return 1;
    }

    /* Process each argument */
    for (int i = 1; i < argc; i++)
    {
        char *arg = argv[i];
        char *eq = strchr(arg, '=');

        /* Case 2: alias name=value */
        if (eq)
        {
            *eq = '\0';
            const char *name = arg;
            const char *value = eq + 1;

            LOG("alias: setting %s='%s'", name, value);

            int idx = _builtin_alias_find(name);
            if (idx >= 0)
            {
                free(alias_values[idx]);
                alias_values[idx] = strdup(value);
            }
            else
            {
                if (alias_count >= MAX_ALIASES)
                {
                    terminal_slow_print("alias: too many aliases\n");
                    return 1;
                }

                alias_names[alias_count] = strdup(name);
                alias_values[alias_count] = strdup(value);
                alias_count++;
            }
        }
        else
        {
            /* Case 3: alias name → print that alias */
            int idx = _builtin_alias_find(arg);
            if (idx >= 0)
                _builtin_alias_print(idx);
            else
            {
                terminal_slow_print("alias: ");
                terminal_slow_print(arg);
                terminal_slow_print(" not found\n");
            }
        }
    }

    return 1;
}