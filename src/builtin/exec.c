#include "main.h"
#include "builtin.h"
#include "terminal.h"

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_exec(int argc, char **argv)
{
	LOG("exec: argc=%d", argc);

	if (argc < 2)
	{
		return 0;
	}

	execvp(argv[1], &argv[1]);

	terminal_slow_print("exec: failed to execute: ");
	terminal_slow_print(argv[1]);
	terminal_slow_print("\n");
	LOG("exec error: %s", strerror(errno));

	return 127;
}