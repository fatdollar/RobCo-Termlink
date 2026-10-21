#include "main.h"
#include "builtin.h"
#include "terminal.h"

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_fg(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    LOG("fg: stub called");
    terminal_slow_print("fg: job control not implemented\n");

    return 1;
}