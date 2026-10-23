#include "main.h"
#include "terminal.h"
#include "builtin.h"

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_unset(int argc, char **argv)
{
	// POSIX: If no arguments are provided, unset does nothing and returns success (0)
	if (argc < 2)
	{
		return 0;
	}

	int exit_status = 0;

	// Loop through all arguments to support unsetting multiple variables (e.g., unset A B C)
	for (int i = 1; i < argc; i++)
	{
		const char *name = argv[i];

		// Note: If you implement a shell variable flags table tracking 'readonly',
		// you would check it here and fail if the variable is readonly.

		if (unsetenv(name) != 0)
		{
			fprintf(stderr, "unset: failed to unset '%s'\n", name);
			exit_status = 1;
		}
	}

	return exit_status;
}