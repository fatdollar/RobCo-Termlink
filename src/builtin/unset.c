#include "terminal.h"
#include "builtin.h"

int builtin_unset(int argc, char **argv)
{
    if (argc < 2)
    {
        terminal_slow_print("unset: usage: unset NAME");
        return 1;
    }
    if (unsetenv(argv[1]) != 0)
    {
        terminal_slow_print("unset: failed to unset variable");
    }
    return 1;
}
