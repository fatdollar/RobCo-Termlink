#include "main.h"
#include "builtin.h"
#include "terminal.h"
#include "input.h"

/* ============================================================
 * GLOBAL STATE
 * ============================================================ */

extern char *positional_params[256];
extern int positional_count;

extern void terminal_process(input_state *st, const char *cmd);

static input_state _source_state;

/* ============================================================
 *  COMMAND
 * ============================================================ */

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

	/* Save old positional parameters safely by duplicating strings */
	char *old_params[256];
	int old_count = positional_count;

	for (int i = 0; i < old_count; i++)
		old_params[i] = positional_params[i] ? strdup(positional_params[i]) : NULL;

	/* Set new positional parameters if args were provided */
	if (argc > 2)
	{
		// Clear current active params first
		for (int i = 0; i < positional_count; i++)
		{
			free(positional_params[i]);
			positional_params[i] = NULL;
		}

		positional_count = argc - 2;
		for (int i = 0; i < positional_count; i++)
			positional_params[i] = strdup(argv[i + 2]);
	}

	char line[4096];
	int last_status = 0;

	while (fgets(line, sizeof(line), fp))
	{
		/* Trim newline */
		size_t len = strlen(line);
		if (len > 0 && line[len - 1] == '\n')
			line[len - 1] = '\0';

		LOG("source: executing line='%s'", line);

		/* Execute using your existing command processor */
		terminal_process(&_source_state, line);
		// If terminal_process tracks exit status, capture it here if available.
	}

	fclose(fp);

	/* Clean up current positional parameters set during sourcing */
	for (int i = 0; i < positional_count; i++)
	{
		free(positional_params[i]);
		positional_params[i] = NULL;
	}

	/* Restore old positional parameters */
	positional_count = old_count;
	for (int i = 0; i < old_count; i++)
	{
		positional_params[i] = old_params[i];
	}

	LOG("source: done");

	return last_status; // POSIX: returns exit status of last command or 0
}