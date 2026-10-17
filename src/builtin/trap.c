#include "main.h"
#include "builtin.h"
#include "terminal.h"

/* forward declaration from your command processor */
extern void terminal_process(input_state *st, const char *cmd);

/* dummy input state for executing trap commands */
static input_state trap_state;

/* maximum signals we support */
#define MAX_TRAPS 64

static char *trap_cmds[MAX_TRAPS];

/* queue for pending traps */
static int pending_signals[MAX_TRAPS];
static int pending_count = 0;

/* enqueue a signal to be processed in main loop */
static void enqueue_signal(int sig)
{
    if (pending_count < MAX_TRAPS)
        pending_signals[pending_count++] = sig;
}

/* signal handler */
static void trap_handler(int sig)
{
    LOG("trap: caught signal %d", sig);
    enqueue_signal(sig);
}

/* install handler for a signal */
static void install_handler(int sig)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = trap_handler;
    sigaction(sig, &sa, NULL);
}

/* process pending traps (called from main loop) */
void trap_process_pending(void)
{
    for (int i = 0; i < pending_count; i++)
    {
        int sig = pending_signals[i];
        if (trap_cmds[sig])
        {
            LOG("trap: executing handler for %d: %s", sig, trap_cmds[sig]);
            terminal_process(&trap_state, trap_cmds[sig]);
        }
    }
    pending_count = 0;
}

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
        return 1;
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

        return 1;
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

        install_handler(sig);

        return 1;
    }

    terminal_slow_print("trap: usage: trap [cmd] [signal]\n");
    return 1;
}