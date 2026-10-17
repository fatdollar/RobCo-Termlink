#include "main.h"
#include "builtin.h"
#include "terminal.h"

int builtin_cd(int argc, char **argv)
{
    LOG("cd: argc=%d argv1='%s'", argc, (argc > 1 ? argv[1] : "(none)"));

    const char *target = NULL;

    /* Case 1: cd with no args → HOME */
    if (argc == 1)
    {
        LOG("cd: no args, using HOME");
        target = getenv("HOME");
        LOG("cd: HOME='%s'", target ? target : "(null)");

        if (!target)
        {
            terminal_slow_print("cd: HOME not set");
            LOG("cd: HOME not set, abort");
            return 1;
        }
    }
    /* Case 2: cd - → OLDPWD */
    else if (strcmp(argv[1], "-") == 0)
    {
        LOG("cd: using OLDPWD");
        target = getenv("OLDPWD");
        LOG("cd: OLDPWD='%s'", target ? target : "(null)");

        if (!target)
        {
            terminal_slow_print("cd: OLDPWD not set");
            LOG("cd: OLDPWD not set, abort");
            return 1;
        }

        /* Print OLDPWD after switching */
        terminal_slow_print(target);
    }
    /* Case 3: cd DIR */
    else
    {
        LOG("cd: using explicit dir '%s'", argv[1]);
        target = argv[1];
    }

    /* Expand ~ to HOME dir path */
    LOG("cd: before tilde expansion target='%s'", target);
    terminal_expand_tilde(&target);
    LOG("cd: after tilde expansion target='%s'", target);

    /* Save old PWD before changing */
    const char *oldpwd = getenv("PWD");
    LOG("cd: oldpwd='%s'", oldpwd ? oldpwd : "(null)");
    if (!oldpwd)
        oldpwd = "";

    /* Attempt to change directory */
    LOG("cd: attempting chdir('%s')", target);
    if (chdir(target) != 0)
    {
        LOG("cd: chdir failed");
        char buf[256];
        snprintf(buf, sizeof(buf), "cd: cannot change directory to %s", target);
        terminal_slow_print(buf);
        return 1;
    }

    LOG("cd: chdir succeeded");

    /* Update OLDPWD */
    LOG("cd: setting OLDPWD='%s'", oldpwd);
    setenv("OLDPWD", oldpwd, 1);

    /* Update PWD */
    char newpwd[PATH_MAX];
    if (getcwd(newpwd, sizeof(newpwd)))
    {
        LOG("cd: newpwd='%s'", newpwd);
        setenv("PWD", newpwd, 1);
    }
    else
    {
        LOG("cd: getcwd failed, fallback PWD='%s'", target);
        setenv("PWD", target, 1); /* fallback */
    }

    if (argc == 1 || strcmp(argv[1], "-") != 0)
    {
        const char *pwd = getenv("PWD");
        LOG("cd: printing new PWD='%s'", pwd ? pwd : "(null)");
        terminal_slow_print(pwd);
    }

    LOG("cd: done");
    return 1;
}