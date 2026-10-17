#include "terminal.h"
#include "builtin.h"

int builtin_clear(int argc, char **argv)
{
    terminal_clear_scroll();
    return 1;
}