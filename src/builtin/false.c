#include "main.h"
#include "builtin.h"
#include "terminal.h"

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_false(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	LOG("false: called");
	return 1;
}