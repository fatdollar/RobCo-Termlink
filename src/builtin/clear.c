#include "terminal.h"
#include "builtin.h"

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_clear(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	terminal_scroll_clear();
	return 0; // Success
}