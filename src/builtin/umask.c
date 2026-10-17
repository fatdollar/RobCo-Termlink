#include "main.h"
#include "builtin.h"
#include "terminal.h"

/* Convert umask to symbolic form: u=rwx,g=rx,o=rx */
static void umask_symbolic(mode_t mask)
{
    char out[64];

    /* permissions are inverted: mask bits disable permissions */
    mode_t perms = 0777 & ~mask;

    char u[4], g[4], o[4];

    snprintf(u, sizeof(u),
             "%c%c%c",
             (perms & S_IRUSR) ? 'r' : '-',
             (perms & S_IWUSR) ? 'w' : '-',
             (perms & S_IXUSR) ? 'x' : '-');

    snprintf(g, sizeof(g),
             "%c%c%c",
             (perms & S_IRGRP) ? 'r' : '-',
             (perms & S_IWGRP) ? 'w' : '-',
             (perms & S_IXGRP) ? 'x' : '-');

    snprintf(o, sizeof(o),
             "%c%c%c",
             (perms & S_IROTH) ? 'r' : '-',
             (perms & S_IWOTH) ? 'w' : '-',
             (perms & S_IXOTH) ? 'x' : '-');

    snprintf(out, sizeof(out),
             "u=%s,g=%s,o=%s\n", u, g, o);

    terminal_slow_print(out);
}

int builtin_umask(int argc, char **argv)
{
    LOG("umask: argc=%d", argc);

    mode_t oldmask = umask(0);
    umask(oldmask); /* restore */

    /* Case 1: no args → print octal mask */
    if (argc == 1)
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%04o\n", oldmask);
        terminal_slow_print(buf);
        return 1;
    }

    /* Case 2: symbolic output */
    if (argc == 2 && strcmp(argv[1], "-S") == 0)
    {
        umask_symbolic(oldmask);
        return 1;
    }

    /* Case 3: set new mask */
    const char *arg = argv[1];

    /* must be octal digits */
    for (const char *p = arg; *p; p++)
    {
        if (*p < '0' || *p > '7')
        {
            terminal_slow_print("umask: invalid mask\n");
            return 1;
        }
    }

    mode_t newmask = strtol(arg, NULL, 8);
    LOG("umask: setting new mask=%04o", newmask);

    umask(newmask);

    return 1;
}