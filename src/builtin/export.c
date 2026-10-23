#include "main.h"
#include "terminal.h"
#include "builtin.h"

/* ============================================================
 *  STATE
 * ============================================================ */

extern char **environ;

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_export(int argc, char **argv)
{
	// POSIX: If no arguments are provided, print all exported environment variables
	if (argc < 2)
	{
		for (char **env = environ; *env != 0; env++)
		{
			// Format typically expected or simple print
			printf("declare -x %s\n", *env);
		}
		fflush(stdout);
		return 0; // Success
	}

	int exit_status = 0;

	// Loop through all arguments to support multiple exports (e.g., export A=1 B=2 C)
	for (int i = 1; i < argc; i++)
	{
		char *arg = argv[i];
		char *eq = strchr(arg, '=');

		if (eq)
		{
			// Case 1: Assignment form (export NAME=VALUE)
			*eq = '\0';
			const char *name = arg;
			const char *value = eq + 1;

			if (setenv(name, value, 1) != 0)
			{
				fprintf(stderr, "export: failed to set variable '%s'\n", name);
				exit_status = 1;
			}
		}
		else
		{
			// Case 2: Mark existing variable for export (export NAME)
			// POSIX states that if no value is given, the variable's export attribute is set.
			char *val = getenv(arg);
			if (val != NULL)
			{
				// Re-setting it via setenv ensures it's marked in the active environment block
				if (setenv(arg, val, 1) != 0)
				{
					fprintf(stderr, "export: failed to export '%s'\n", arg);
					exit_status = 1;
				}
			}
			else
			{
				// If it doesn't exist yet, POSIX allows creating it as empty or leaving it.
				// Setting it to an empty string matches standard bash behavior.
				if (setenv(arg, "", 1) != 0)
				{
					fprintf(stderr, "export: failed to export '%s'\n", arg);
					exit_status = 1;
				}
			}
		}
	}

	return exit_status;
}