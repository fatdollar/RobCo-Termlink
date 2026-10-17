#include "terminal.h"
#include "builtin.h"

int builtin_export(int argc, char **argv)
{
	if (argc < 2)
	{
		terminal_slow_print("export: usage: export NAME=VALUE");
		return 1;
	}
	char *eq = strchr(argv[1], '=');
	if (!eq)
	{
		terminal_slow_print("export: usage: export NAME=VALUE");
		return 1;
	}
	*eq = '\0';
	const char *name = argv[1];
	const char *value = eq + 1;
	if (setenv(name, value, 1) != 0)
	{
		terminal_slow_print("export: failed to set variable");
	}
	return 1;
}