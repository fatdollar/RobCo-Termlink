#include "main.h"
#include "builtin.h"
#include "terminal.h"

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_break(int argc, char **argv)
{
	LOG("break: argc=%d", argc);
	(void)argc;
	(void)argv;
	terminal_slow_print("break: only valid in loops\n");
	return 1;
}