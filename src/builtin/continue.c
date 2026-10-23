#include "main.h"
#include "builtin.h"
#include "terminal.h"

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_continue(int argc, char **argv)
{
	LOG("continue: argc=%d", argc);
	(void)argc;
	(void)argv;
	terminal_slow_print("continue: only valid in loops\n");
	return 1;
}