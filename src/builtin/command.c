#include "main.h"
#include "builtin.h"
#include "terminal.h"

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_command(int argc, char **argv)
{
	LOG("command: argc=%d", argc);

	if (argc < 2)
	{
		terminal_slow_print("command: usage: command [-p] command [args...]\n");
		return 1;
	}

	int start = 1;
	if (strcmp(argv[1], "-p") == 0 || strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "-V") == 0)
	{
		start = 2;
	}

	if (start >= argc)
	{
		return 0;
	}

	execvp(argv[start], &argv[start]);

	terminal_slow_print("command: not found: ");
	terminal_slow_print(argv[start]);
	terminal_slow_print("\n");

	return 127;
}