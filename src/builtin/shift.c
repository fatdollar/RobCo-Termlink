#include "main.h"
#include "builtin.h"
#include "terminal.h"

/* ============================================================
 * GLOBAL STATE
 * ============================================================ */

extern char *positional_params[256];
extern int positional_count;

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_shift(int argc, char **argv)
{
	LOG("shift: argc=%d", argc);

	int n = 1; /* default shift amount */

	if (argc > 1)
	{
		n = atoi(argv[1]);
		if (n < 0)
		{
			terminal_slow_print("shift: invalid shift count\n");
			return 1;
		}
	}

	LOG("shift: shifting by %d", n);

	/* Shifting by 0 is a successful no-op */
	if (n == 0)
		return 0;

	if (positional_count == 0)
	{
		terminal_slow_print("shift: no positional parameters\n");
		return 1;
	}

	if (n > positional_count)
	{
		terminal_slow_print("shift: count exceeds positional parameters\n");
		return 1;
	}

	if (n == positional_count)
	{
		/* remove all parameters successfully */
		LOG("shift: removing all positional parameters");

		for (int i = 0; i < positional_count; i++)
			free(positional_params[i]);

		positional_count = 0;
		return 0;
	}

	/* free first n parameters */
	for (int i = 0; i < n; i++)
	{
		LOG("shift: freeing param[%d]='%s'", i, positional_params[i]);
		free(positional_params[i]);
	}

	/* shift remaining parameters left */
	for (int i = n; i < positional_count; i++)
	{
		positional_params[i - n] = positional_params[i];
		LOG("shift: param[%d] now '%s'", i - n, positional_params[i]);
	}

	positional_count -= n;

	return 0; // Success
}