#include "main.h"
#include "builtin.h"
#include "terminal.h"

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_kill(int argc, char **argv)
{
	LOG("kill: argc=%d", argc);

	if (argc < 2)
	{
		terminal_slow_print("kill: usage: kill [-s sig_name | -sig_num] pid...\n");
		return 1;
	}

	int sig = SIGTERM; // Default signal per POSIX
	int start_index = 1;

	/* Check for signal option (e.g., -9, -SIGKILL, or -s KILL) */
	if (argv[1][0] == '-')
	{
		if (argv[1][1] == 's' && argc > 2)
		{
			// Handle -s SIGNAME or -s name
			// (For simplicity, checking common names or numeric values)
			if (strcasecmp(argv[2], "KILL") == 0 || strcmp(argv[2], "9") == 0)
				sig = SIGKILL;
			else if (strcasecmp(argv[2], "TERM") == 0 || strcmp(argv[2], "15") == 0)
				sig = SIGTERM;
			else if (strcasecmp(argv[2], "INT") == 0 || strcmp(argv[2], "2") == 0)
				sig = SIGINT;
			else if (strcasecmp(argv[2], "STOP") == 0)
				sig = SIGSTOP;
			else if (strcasecmp(argv[2], "CONT") == 0)
				sig = SIGCONT;
			else
				sig = atoi(argv[2]); // Fallback to raw numeric string

			start_index = 3;
		}
		else
		{
			// Handle shorthand format like -9 or -SIGKILL
			const char *sig_str = &argv[1][1];
			if (strcasecmp(sig_str, "KILL") == 0)
				sig = SIGKILL;
			else if (strcasecmp(sig_str, "TERM") == 0)
				sig = SIGTERM;
			else if (strcasecmp(sig_str, "INT") == 0)
				sig = SIGINT;
			else if (strcasecmp(sig_str, "STOP") == 0)
				sig = SIGSTOP;
			else if (strcasecmp(sig_str, "CONT") == 0)
				sig = SIGCONT;
			else
				sig = atoi(sig_str); // e.g., -9 -> 9

			start_index = 2;
		}
	}

	if (start_index >= argc)
	{
		terminal_slow_print("kill: option requires an argument\n");
		return 1;
	}

	int exit_status = 0;

	for (int i = start_index; i < argc; i++)
	{
		pid_t pid = (pid_t)atoi(argv[i]);
		LOG("kill: sending signal %d to %d", sig, (int)pid);

		if (kill(pid, sig) != 0)
		{
			terminal_slow_print("kill: failed to kill ");
			terminal_slow_print(argv[i]);
			terminal_slow_print("\n");
			exit_status = 1;
		}
	}

	return exit_status;
}