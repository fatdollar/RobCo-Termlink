#include "main.h"
#include "builtin.h"
#include "terminal.h"

#define MAX_TRAPS 64

/* ============================================================
 * GLOBAL STATE
 * ============================================================ */

static char *trap_cmds[MAX_TRAPS];

static int signals[MAX_TRAPS];
static int signal_count = 0;

/* ============================================================
 *  FORWARD DECLARATIONS
 * ============================================================ */

static void _builtin_trap_enqueue_signal(int sig);
static void _builtin_trap_handler(int sig);
static void _builtin_trap_install_handler(int sig);

/* ============================================================
 *  INTERNAL HELPERS
 * ============================================================ */

static void _builtin_trap_enqueue_signal(int sig)
{
	if (signal_count < MAX_TRAPS)
		signals[signal_count++] = sig;
}

static void _builtin_trap_handler(int sig)
{
	LOG("trap: caught signal %d", sig);
	_builtin_trap_enqueue_signal(sig);
}

static void _builtin_trap_install_handler(int sig)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = _builtin_trap_handler;
	sigaction(sig, &sa, NULL);
}

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_trap(int argc, char **argv)
{
	LOG("trap: argc=%d", argc);

	/* Case 1: print all traps */
	if (argc == 1)
	{
		for (int sig = 1; sig < MAX_TRAPS; sig++)
		{
			if (trap_cmds[sig])
			{
				terminal_slow_print("trap '");
				terminal_slow_print(trap_cmds[sig]);
				terminal_slow_print("' ");
				char buf[16];
				snprintf(buf, sizeof(buf), "%d\n", sig);
				terminal_slow_print(buf);
			}
		}
		return 0; // Success
	}

	/* Case 2: trap '' SIG → clear trap */
	if (argc == 3 && strcmp(argv[1], "") == 0)
	{
		int sig = atoi(argv[2]);
		if (sig <= 0 || sig >= MAX_TRAPS)
		{
			terminal_slow_print("trap: invalid signal\n");
			return 1;
		}

		LOG("trap: clearing trap for %d", sig);

		free(trap_cmds[sig]);
		trap_cmds[sig] = NULL;

		/* restore default handler */
		struct sigaction sa;
		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = SIG_DFL;
		sigaction(sig, &sa, NULL);

		return 0; // Success
	}

	/* Case 3: trap 'cmd' SIG → set trap */
	if (argc == 3)
	{
		const char *cmd = argv[1];
		int sig = atoi(argv[2]);

		if (sig <= 0 || sig >= MAX_TRAPS)
		{
			terminal_slow_print("trap: invalid signal\n");
			return 1;
		}

		LOG("trap: setting trap for %d: %s", sig, cmd);

		free(trap_cmds[sig]);
		trap_cmds[sig] = strdup(cmd);

		_builtin_trap_install_handler(sig);

		return 0; // Success
	}

	terminal_slow_print("trap: usage: trap [cmd] [signal]\n");
	return 1;
}