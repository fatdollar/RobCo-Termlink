#include "main.h"
#include "builtin.h"
#include "terminal.h"
#include "util.h"
#include "history.h"
#include "input.h"
#include "input_state.h"

char builtin_external_cmds[MAX_EXTERNAL_CMDS][PATH_MAX];
int builtin_external_cmd_count = 0;

static void expand_tilde(const char **target)
{
    if ((*target)[0] == '~')
    {
        const char *home = getenv("HOME");
        if (!home)
            home = "/";

        static char expanded[PATH_MAX];

        if ((*target)[1] == '/' || (*target)[1] == '\0')
        {
            // "~" or "~/something"
            snprintf(expanded, sizeof(expanded), "%s%s", home, (*target) + 1);
            *target = expanded;
        }
        else
        {
            // "~user" (optional, skip for now)
            // For now, treat "~user" as literal.
        }
    }
}

void builtin_register_external(const char *name)
{
	if (builtin_external_cmd_count >= MAX_EXTERNAL_CMDS)
		return;
	snprintf(builtin_external_cmds[builtin_external_cmd_count],
			 sizeof(builtin_external_cmds[builtin_external_cmd_count]),
			 "%s", name);
	builtin_external_cmd_count++;
}

void builtin_load_commands_from_dir(const char *dirpath)
{
	DIR *d = opendir(dirpath);
	if (!d)
		return;

	struct dirent *ent;
	while ((ent = readdir(d)) != NULL)
	{
		char fullpath[PATH_MAX];
		snprintf(fullpath, sizeof(fullpath), "%s/%s", dirpath, ent->d_name);

		if (util_is_executable(fullpath))
		{
			builtin_register_external(ent->d_name);
		}
	}

	closedir(d);
}

void builtin_load_external_from_path(void)
{
#ifdef TL_EXTERNAL_CMDS
	const char *path = getenv("PATH");
	if (!path)
		return;

	char temp[4096];
	snprintf(temp, sizeof(temp), "%s", path);

	char *saveptr;
	char *dir = strtok_r(temp, ":", &saveptr);

	while (dir)
	{
		builtin_load_commands_from_dir(dir);
		dir = strtok_r(NULL, ":", &saveptr);
	}
#endif
}

#ifdef TL_CD
int builtin_cd(int argc, char **argv)
{
    const char *target = NULL;

    /* Case 1: cd with no args → HOME */
    if (argc == 1)
    {
        target = getenv("HOME");
        if (!target)
        {
            terminal_slow_print("cd: HOME not set");
            return 1;
        }
    }
    /* Case 2: cd - → OLDPWD */
    else if (strcmp(argv[1], "-") == 0)
    {
        target = getenv("OLDPWD");
        if (!target)
        {
            terminal_slow_print("cd: OLDPWD not set");
            return 1;
        }

        /* Print OLDPWD after switching */
        terminal_slow_print(target);
    }
    /* Case 3: cd DIR */
    else
    {
        target = argv[1];
    }

	/* Expand ~ to HOME dir path */
	expand_tilde(&target);

    /* Save old PWD before changing */
    const char *oldpwd = getenv("PWD");
    if (!oldpwd)
        oldpwd = "";

    /* Attempt to change directory */
    if (chdir(target) != 0)
    {
        char buf[256];
        snprintf(buf, sizeof(buf), "cd: cannot change directory to %s", target);
        terminal_slow_print(buf);
        return 1;
    }

    /* Update OLDPWD */
    setenv("OLDPWD", oldpwd, 1);

    /* Update PWD */
    char newpwd[PATH_MAX];
    if (getcwd(newpwd, sizeof(newpwd)))
	{
        setenv("PWD", newpwd, 1);
	}
    else
	{
        setenv("PWD", target, 1);  /* fallback */
	}

	if (argc == 1 || strcmp(argv[1], "-") != 0)
	{
		terminal_slow_print(getenv("PWD"));
	}

    return 1;
}
#endif

int builtin_exit(int argc, char **argv)
{
	terminal_slow_print("Shutting down terminal...");
	exit(0);
	return 1;
}

int builtin_help(int argc, char **argv)
{
	terminal_slow_print("Available commands:");

	terminal_slow_print("Built-ins:");
	terminal_slow_print("  help");
	terminal_slow_print("  clear");
	terminal_slow_print("  exit");
	terminal_slow_print("  cd");
	terminal_slow_print("  export");
	terminal_slow_print("  unset");

	terminal_slow_print("");
	terminal_slow_print("External commands:");

	for (int i = 0; i < builtin_external_cmd_count; i++)
	{
		terminal_slow_print(builtin_external_cmds[i]);
	}
	return 1;
}

int builtin_clear(int argc, char **argv)
{
	terminal_clear_scroll();
	return 1;
}

int builtin_export(int argc, char **argv)
{
	if (argc < 2)
	{
		terminal_slow_print("export: usage: export NAME=VALUE");
		return 1;
	}
	char *eq = strchr(argv[1], '=');
	if (!eq)
	{
		terminal_slow_print("export: usage: export NAME=VALUE");
		return 1;
	}
	*eq = '\0';
	const char *name = argv[1];
	const char *value = eq + 1;
	if (setenv(name, value, 1) != 0)
	{
		terminal_slow_print("export: failed to set variable");
	}
	return 1;
}

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

int builtin_try(int argc, char **argv)
{
    if (argc == 0)
        return 1;

#ifdef TL_CD
    if (strcmp(argv[0], "cd") == 0)
        return builtin_cd(argc, argv);
#endif

#ifdef TL_EXIT
    if (strcmp(argv[0], "exit") == 0)
        return builtin_exit(argc, argv);
#endif

#ifdef TL_CLEAR
    if (strcmp(argv[0], "clear") == 0)
        return builtin_clear(argc, argv);
#endif

#ifdef TL_EXPORT
    if (strcmp(argv[0], "export") == 0)
        return builtin_export(argc, argv);
#endif

#ifdef TL_UNSET
    if (strcmp(argv[0], "unset") == 0)
        return builtin_unset(argc, argv);
#endif

    if (strcmp(argv[0], "help") == 0)
        return builtin_help(argc, argv);

    return 0;  // not a builtin
}

void builtin_run_external(char **argv)
{
#ifndef TL_EXTERNAL_CMDS
    terminal_slow_print("External commands are disabled.");
    return;
#endif
	int pipefd[2];
	if (pipe(pipefd) < 0)
	{
		terminal_slow_print("Error: pipe failed");
		return;
	}

	pid_t pid = fork();
	if (pid < 0)
	{
		terminal_slow_print("Error: fork failed");
		return;
	}

	if (pid == 0)
	{
		close(pipefd[0]);

		dup2(pipefd[1], STDOUT_FILENO);
		dup2(pipefd[1], STDERR_FILENO);
		close(pipefd[1]);

		execvp(argv[0], argv);
		perror("execvp");
		exit(1);
	}

	close(pipefd[1]);

	char buffer[256];
	ssize_t n;

	while ((n = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0)
	{
		buffer[n] = '\0';
		terminal_slow_print(buffer);
	}

	close(pipefd[0]);

	int status;
	waitpid(pid, &status, 0);
}

void builtin_process(input_state *st, const char *cmd)
{
	char buf[PATH_MAX];
	char *argv[MAX_ARGS];
	int argc;

	snprintf(buf, sizeof(buf), "%s", cmd);

	argc = input_parse_args(buf, argv, MAX_ARGS);
	if (argc == 0)
		return;

	// always clear before executing a command
	terminal_clear_scroll();
	history_add(&st->hist, cmd);
	history_save(&st->hist);
	terminal_cursor_line = terminal_scroll_top;

	if (builtin_try(argc, argv))
	{
		terminal_draw_prompt();
		return;
	}

	builtin_run_external(argv);
	terminal_draw_prompt();
}