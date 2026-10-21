#include "terminal.h"
#include "builtin.h"

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_clear(int argc, char **argv)
{
    terminal_scroll_clear();
    return 1;
}