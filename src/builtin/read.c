#include "main.h"
#include "builtin.h"
#include "terminal.h"
#include "input.h"

int builtin_read(int argc, char **argv)
{
    LOG("read: argc=%d", argc);

    const char *varname = "REPLY";
    if (argc > 1)
        varname = argv[1];

    LOG("read: reading into '%s'", varname);

    /* Prepare temporary input state */
    input_state st;
    memset(&st, 0, sizeof(st));

    char buf[1024];
    memset(buf, 0, sizeof(buf));

    /* Use your existing interactive line reader */
    input_read_line(&st, buf, sizeof(buf));

    LOG("read: raw='%s'", buf);

    /* Trim newline if present */
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n')
        buf[len - 1] = '\0';

    LOG("read: trimmed='%s'", buf);

    /* Store in environment */
    if (setenv(varname, buf, 1) != 0)
    {
        terminal_slow_print("read: failed to set variable\n");
        return 1;
    }

    return 1;
}