#include "main.h"
#include "builtin.h"
#include "terminal.h"

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_colon(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	LOG("colon: no-op called");
	return 0;
}