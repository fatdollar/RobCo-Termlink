#include "main.h"
#include "builtin.h"
#include "terminal.h"

int builtin_kill(int argc, char **argv)
{
    LOG("kill: argc=%d", argc);

    if (argc < 2)
    {
        terminal_slow_print("kill: usage: kill PID...\n");
        return 1;
    }

    for (int i = 1; i < argc; i++)
    {
        pid_t pid = (pid_t)atoi(argv[i]);
        LOG("kill: sending SIGTERM to %d", (int)pid);

        if (kill(pid, SIGTERM) != 0)
        {
            terminal_slow_print("kill: failed to kill ");
            terminal_slow_print(argv[i]);
            terminal_slow_print("\n");
        }
    }

    return 1;
}