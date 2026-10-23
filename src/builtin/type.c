#include "main.h"
#include "builtin.h"
#include "terminal.h"

/* ============================================================
 * GLOBAL STATE
 * ============================================================ */

extern builtin_entry builtins[MAX_BUILTINS];
extern int builtin_count;

extern char external_cmds[][PATH_MAX];
extern int external_cmd_count;

/* ============================================================
 *  FORWARD DECLARATIONS
 * ============================================================ */

static int _builtin_type_is_builtin(const char *name);
static const char *_builtin_type_find_external(const char *name);

/* ============================================================
 *  INTERNAL HELPERS
 * ============================================================ */

static int _builtin_type_is_builtin(const char *name)
{
	for (int i = 0; i < builtin_count; i++)
	{
		if (strcmp(builtins[i].name, name) == 0)
			return 1;
	}
	return 0;
}

static const char *_builtin_type_find_external(const char *name)
{
	for (int i = 0; i < external_cmd_count; i++)
	{
		if (strcmp(external_cmds[i], name) == 0)
			return external_cmds[i];
	}
	return NULL;
}

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_type(int argc, char **argv)
{
	LOG("type: argc=%d", argc);

	if (argc < 2)
	{
		terminal_slow_print("type: usage: type NAME...\n");
		return 1;
	}

	int exit_status = 0;

	/* Loop through all provided names to support multiple arguments */
	for (int i = 1; i < argc; i++)
	{
		const char *name = argv[i];
		LOG("type: checking '%s'", name);

		int found = 0;

		/* 1. Check builtin */
		if (_builtin_type_is_builtin(name))
		{
			terminal_slow_print(name);
			terminal_slow_print(" is a shell builtin\n");
			found = 1;
		}
		else
		{
			/* 2. Check external commands */
			const char *ext = _builtin_type_find_external(name);
			if (ext)
			{
				terminal_slow_print(name);
				terminal_slow_print(" is ");
				terminal_slow_print(ext);
				terminal_slow_print("\n");
				found = 1;
			}
			else
			{
				/* 3. Check PATH manually (POSIX behavior) */
				const char *path = getenv("PATH");
				if (path)
				{
					char temp[4096];
					snprintf(temp, sizeof(temp), "%s", path);

					char *saveptr;
					char *dir = strtok_r(temp, ":", &saveptr);

					while (dir)
					{
						char full[PATH_MAX];
						snprintf(full, sizeof(full), "%s/%s", dir, name);

						if (access(full, X_OK) == 0)
						{
							terminal_slow_print(name);
							terminal_slow_print(" is ");
							terminal_slow_print(full);
							terminal_slow_print("\n");
							found = 1;
							break;
						}

						dir = strtok_r(NULL, ":", &saveptr);
					}
				}
			}
		}

		/* 4. Not found anywhere */
		if (!found)
		{
			terminal_slow_print(name);
			terminal_slow_print(" not found\n");
			exit_status = 1; // Mark failure if any name wasn't found
		}
	}

	return exit_status;
}