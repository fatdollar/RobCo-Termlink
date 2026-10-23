#include "main.h"
#include "builtin.h"
#include "terminal.h"
#include "input.h"

static input_state _eval_state;

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_eval(int argc, char **argv)
{
	LOG("eval: argc=%d", argc);

	if (argc < 2)
		return 0; // POSIX: empty eval returns 0

	size_t total_len = 0;
	for (int i = 1; i < argc; i++)
	{
		total_len += strlen(argv[i]) + 1;
	}

	char *cmd_buf = malloc(total_len);
	if (!cmd_buf)
	{
		terminal_slow_print("eval: out of memory\n");
		return 1;
	}

	cmd_buf[0] = '\0';
	for (int i = 1; i < argc; i++)
	{
		strcat(cmd_buf, argv[i]);
		if (i < argc - 1)
			strcat(cmd_buf, " ");
	}

	LOG("eval: executing '%s'", cmd_buf);
	terminal_process(&_eval_state, cmd_buf);

	free(cmd_buf);
	return 0;
}