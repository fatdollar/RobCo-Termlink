#include "terminal.h"
#include "builtin.h"

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_exit(int argc, char **argv)
{
    terminal_slow_print_at(terminal_rows, "Shutting down...\n");
    exit(0);
    return 1;
}