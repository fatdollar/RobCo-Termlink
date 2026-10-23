#include "main.h"
#include "builtin.h"
#include "terminal.h"

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_true(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	LOG("true: called");
	return 0;
}