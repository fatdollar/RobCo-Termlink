#include "main.h"
#include "builtin.h"
#include "terminal.h"

#define MAX_ALIASES 256

/* alias table from alias.c */
extern char *alias_names[MAX_ALIASES];
extern char *alias_values[MAX_ALIASES];
extern int alias_count;

/* Remove alias at index */
static void alias_remove(int idx)
{
    LOG("unalias: removing index %d", idx);

    free(alias_names[idx]);
    free(alias_values[idx]);

    /* shift remaining aliases left */
    for (int i = idx + 1; i < alias_count; i++)
    {
        alias_names[i - 1] = alias_names[i];
        alias_values[i - 1] = alias_values[i];
    }

    alias_count--;
}

int builtin_unalias(int argc, char **argv)
{
    LOG("unalias: argc=%d", argc);

    if (argc < 2)
    {
        terminal_slow_print("unalias: usage: unalias [-a] name...\n");
        return 1;
    }

    /* Case 1: unalias -a → remove all aliases */
    if (strcmp(argv[1], "-a") == 0)
    {
        LOG("unalias: removing all aliases");

        for (int i = 0; i < alias_count; i++)
        {
            free(alias_names[i]);
            free(alias_values[i]);
        }

        alias_count = 0;
        return 1;
    }

    /* Case 2: unalias name... */
    for (int i = 1; i < argc; i++)
    {
        const char *name = argv[i];
        LOG("unalias: removing '%s'", name);

        int found = 0;

        for (int j = 0; j < alias_count; j++)
        {
            if (strcmp(alias_names[j], name) == 0)
            {
                alias_remove(j);
                found = 1;
                break;
            }
        }

        if (!found)
        {
            terminal_slow_print("unalias: ");
            terminal_slow_print(name);
            terminal_slow_print(" not found\n");
        }
    }

    return 1;
}