#include "terminal.h"
#include "builtin.h"

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_exit(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	terminal_slow_print_at(terminal_rows, "Shutting down...\n");
	exit(0);
}