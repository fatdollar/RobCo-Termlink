#include "main.h"
#include "builtin.h"
#include "terminal.h"

/* ============================================================
 * GLOBAL STATE
 * ============================================================ */

#define MAX_POS_PARAMS 256

char *positional_params[MAX_POS_PARAMS];
int positional_count = 0;

/* ============================================================
 *  COMMAND
 * ============================================================ */

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
		fflush(stdout);
		return 0; // Success
	}

	int start_index = 1;

	/* Case 2: Explicit `--` to separate options from arguments */
	if (strcmp(argv[1], "--") == 0)
	{
		start_index = 2; // Positional parameters start after `--`
	}
	/* Case 3: Option flags (e.g., -e, +e) */
	else if (argv[1][0] == '-' || argv[1][0] == '+')
	{
		// If it's a stub or option, handle or report error for now
		terminal_slow_print("set: option handling not fully implemented yet\n");
		return 1;
	}
	/* Case 4: Direct positional parameters without `--` (e.g., set a b c) */
	else
	{
		start_index = 1; // Positional parameters start at argv[1]
	}

	/* Calculate new parameter count */
	int new_count = argc - start_index;
	if (new_count > MAX_POS_PARAMS)
	{
		fprintf(stderr, "set: too many positional parameters (max %d)\n", MAX_POS_PARAMS);
		return 1;
	}

	LOG("set: setting positional parameters, count=%d", new_count);

	/* Clear old positional parameters safely */
	for (int i = 0; i < positional_count; i++)
	{
		free(positional_params[i]);
		positional_params[i] = NULL;
	}

	/* Assign new positional parameters */
	positional_count = new_count;
	for (int i = 0; i < positional_count; i++)
	{
		positional_params[i] = strdup(argv[start_index + i]);
		LOG("set: param[%d]='%s'", i, positional_params[i]);
	}

	return 0; // Success
}