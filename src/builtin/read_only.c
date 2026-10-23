#include "main.h"
#include "builtin.h"
#include "terminal.h"

#define MAX_READONLY 256

/* ============================================================
 *  STATE
 * ============================================================ */

static char *readonly_vars[MAX_READONLY];
static int readonly_count = 0;

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_readonly(int argc, char **argv)
{
	LOG("readonly: argc=%d", argc);

	if (argc == 1)
	{
		for (int i = 0; i < readonly_count; i++)
		{
			terminal_slow_print("readonly ");
			terminal_slow_print(readonly_vars[i]);
			terminal_slow_print("\n");
		}
		return 0;
	}

	for (int i = 1; i < argc; i++)
	{
		char *arg = argv[i];
		char *eq = strchr(arg, '=');
		if (eq)
		{
			*eq = '\0';
			setenv(arg, eq + 1, 1);
		}

		if (readonly_count < MAX_READONLY)
		{
			int exists = 0;
			for (int j = 0; j < readonly_count; j++)
			{
				if (strcmp(readonly_vars[j], arg) == 0)
					exists = 1;
			}
			if (!exists)
			{
				readonly_vars[readonly_count++] = strdup(arg);
				LOG("readonly: marked '%s' as readonly", arg);
			}
		}
	}

	return 0;
}