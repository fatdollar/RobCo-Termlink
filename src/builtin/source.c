#include "main.h"
#include "builtin.h"
#include "terminal.h"
#include "input.h"

/* positional parameters from set.c */
extern char *positional_params[256];
extern int positional_count;

/* forward declaration from your main command processor */
extern void terminal_process(input_state *st, const char *cmd);

/* We need a dummy input_state for executing sourced lines */
static input_state source_state;

int builtin_source(int argc, char **argv)
{
    LOG("source: argc=%d", argc);

    if (argc < 2)
    {
        terminal_slow_print("source: usage: source FILE [args...]\n");
        return 1;
    }

    const char *filename = argv[1];
    LOG("source: opening '%s'", filename);

    FILE *fp = fopen(filename, "r");
    if (!fp)
    {
        terminal_slow_print("source: cannot open file: ");
        terminal_slow_print(filename);
        terminal_slow_print("\n");
        return 1;
    }

    /* Save old positional parameters */
    char *old_params[256];
    int old_count = positional_count;

    for (int i = 0; i < old_count; i++)
        old_params[i] = positional_params[i];

    /* Set new positional parameters if args were provided */
    if (argc > 2)
    {
        positional_count = argc - 2;
        for (int i = 0; i < positional_count; i++)
            positional_params[i] = strdup(argv[i + 2]);
    }

    char line[4096];

    while (fgets(line, sizeof(line), fp))
    {
        /* Trim newline */
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
            line[len - 1] = '\0';

        LOG("source: executing line='%s'", line);

        /* Execute using your existing command processor */
        terminal_process(&source_state, line);
    }

    fclose(fp);

    /* Restore old positional parameters */
    for (int i = 0; i < positional_count; i++)
        free(positional_params[i]);

    positional_count = old_count;

    for (int i = 0; i < old_count; i++)
        positional_params[i] = old_params[i];

    LOG("source: done");

    return 1;
}