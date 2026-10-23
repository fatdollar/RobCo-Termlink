#include "main.h"
#include "builtin.h"
#include "terminal.h"

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_return(int argc, char **argv)
{
	LOG("return: argc=%d", argc);
	int status = 0;
	if (argc > 1)
	{
		status = atoi(argv[1]);
	}
	return status;
}