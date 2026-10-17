#include "terminal.h"
#include "builtin.h"

int builtin_exit(int argc, char **argv)
{
    terminal_slow_print("Shutting down terminal...");
    exit(0);
    return 1;
}