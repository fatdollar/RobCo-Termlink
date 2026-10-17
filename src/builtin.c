#include "main.h"
#include "builtin.h"
#include "terminal.h"
#include "util.h"
#include "history.h"
#include "input.h"
#include "input_state.h"
#include "debug.h"

char builtin_external_cmds[MAX_EXTERNAL_CMDS][PATH_MAX];
int builtin_external_cmd_count = 0;

static void expand_tilde(const char **target)
{
	LOG("expand_tilde: input='%s'", *target);

	if ((*target)[0] == '~')
	{
		LOG("expand_tilde: tilde detected");

		const char *home = getenv("HOME");
		LOG("expand_tilde: HOME='%s'", home ? home : "(null)");

		if (!home)
			home = "/";

		static char expanded[PATH_MAX];

		if ((*target)[1] == '/' || (*target)[1] == '\0')
		{
			// "~" or "~/something"
			LOG("expand_tilde: expanding '~' or '~/'");
			snprintf(expanded, sizeof(expanded), "%s%s", home, (*target) + 1);
			*target = expanded;
			LOG("expand_tilde: expanded='%s'", *target);
		}
		else
		{
			// "~user" (optional, skip for now)
			// For now, treat "~user" as literal.
			LOG("expand_tilde: '~user' form detected, leaving literal");
		}
	}

	LOG("expand_tilde: final='%s'", *target);
}

void builtin_register_external(const char *name)
{
	LOG("register_external: '%s'", name);

	if (builtin_external_cmd_count >= MAX_EXTERNAL_CMDS)
	{
		LOG("register_external: MAX_EXTERNAL_CMDS reached");
		return;
	}

	snprintf(builtin_external_cmds[builtin_external_cmd_count],
			 sizeof(builtin_external_cmds[builtin_external_cmd_count]),
			 "%s", name);

	builtin_external_cmd_count++;

	LOG("register_external: count=%d", builtin_external_cmd_count);
}

void builtin_load_commands_from_dir(const char *dirpath)
{
	LOG("load_commands_from_dir: '%s'", dirpath);

	DIR *d = opendir(dirpath);
	if (!d)
	{
		LOG("load_commands_from_dir: cannot open");
		return;
	}

	struct dirent *ent;
	while ((ent = readdir(d)) != NULL)
	{
		LOG("load_commands_from_dir: found '%s'", ent->d_name);

		char fullpath[PATH_MAX];
		snprintf(fullpath, sizeof(fullpath), "%s/%s", dirpath, ent->d_name);

		if (util_is_executable(fullpath))
		{
			LOG("load_commands_from_dir: executable '%s'", ent->d_name);
			builtin_register_external(ent->d_name);
		}
	}

	closedir(d);
}

void builtin_load_external_from_path(void)
{
#ifdef TL_EXTERNAL_CMDS
	const char *path = getenv("PATH");
	LOG("load_external_from_path: PATH='%s'", path ? path : "(null)");

	if (!path)
		return;

	char temp[4096];
	snprintf(temp, sizeof(temp), "%s", path);

	char *saveptr;
	char *dir = strtok_r(temp, ":", &saveptr);

	while (dir)
	{
		LOG("load_external_from_path: scanning '%s'", dir);
		builtin_load_commands_from_dir(dir);
		dir = strtok_r(NULL, ":", &saveptr);
	}
#endif
}

#ifdef TL_CD
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
	expand_tilde(&target);
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
	LOG("builtin_try: argc=%d cmd='%s'", argc, argv[0]);

#ifdef TL_CD
	if (strcmp(argv[0], "cd") == 0)
	{
		LOG("builtin_try: matched cd");
		return builtin_cd(argc, argv);
	}
#endif

#ifdef TL_EXIT
	if (strcmp(argv[0], "exit") == 0)
	{
		LOG("builtin_try: matched exit");
		return builtin_exit(argc, argv);
	}
#endif

#ifdef TL_CLEAR
	if (strcmp(argv[0], "clear") == 0)
	{
		LOG("builtin_try: matched clear");
		return builtin_clear(argc, argv);
	}
#endif

#ifdef TL_EXPORT
	if (strcmp(argv[0], "export") == 0)
	{
		LOG("builtin_try: matched export");
		return builtin_export(argc, argv);
	}
#endif

#ifdef TL_UNSET
	if (strcmp(argv[0], "unset") == 0)
	{
		LOG("builtin_try: matched unset");
		return builtin_unset(argc, argv);
	}
#endif

	if (strcmp(argv[0], "help") == 0)
	{
		LOG("builtin_try: matched help");
		return builtin_help(argc, argv);
	}

	LOG("builtin_try: not a builtin");
	return 0;
}

void builtin_run_external(char **argv)
{
	LOG("run_external: cmd='%s'", argv[0]);

#ifndef TL_EXTERNAL_CMDS
	LOG("run_external: external commands disabled");
	terminal_slow_print("External commands are disabled.");
	return;
#endif

	int pipefd[2];
	if (pipe(pipefd) < 0)
	{
		LOG("run_external: pipe failed");
		terminal_slow_print("Error: pipe failed");
		return;
	}

	pid_t pid = fork();
	if (pid < 0)
	{
		LOG("run_external: fork failed");
		terminal_slow_print("Error: fork failed");
		return;
	}

	if (pid == 0)
	{
		LOG("run_external: child exec '%s'", argv[0]);
		close(pipefd[0]);

		dup2(pipefd[1], STDOUT_FILENO);
		dup2(pipefd[1], STDERR_FILENO);
		close(pipefd[1]);

		execvp(argv[0], argv);
		perror("execvp");
		exit(1);
	}

	LOG("run_external: parent waiting");

	close(pipefd[1]);

	char buffer[256];
	ssize_t n;

	while ((n = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0)
	{
		buffer[n] = '\0';
		LOG("run_external: output='%s'", buffer);
		terminal_slow_print(buffer);
	}

	close(pipefd[0]);

	int status;
	waitpid(pid, &status, 0);

	LOG("run_external: child exited status=%d", status);
}

void builtin_process(input_state *st, const char *cmd)
{
	LOG("builtin_process: cmd='%s'", cmd);

	char buf[PATH_MAX];
	char *argv[MAX_ARGS];
	int argc;

	snprintf(buf, sizeof(buf), "%s", cmd);
	LOG("builtin_process: buf='%s'", buf);

	argc = input_parse_args(buf, argv, MAX_ARGS);
	LOG("builtin_process: argc=%d", argc);

	if (argc == 0)
	{
		LOG("builtin_process: argc==0, abort");
		return;
	}

	// always clear before executing a command
	LOG("builtin_process: clearing scroll");
	terminal_clear_scroll();

	LOG("builtin_process: adding to history");
	history_add(&st->hist, cmd);

	LOG("builtin_process: saving history");
	history_save(&st->hist);

	terminal_cursor_line = terminal_scroll_top;
	LOG("builtin_process: cursor_line reset to %d", terminal_cursor_line);

	LOG("builtin_process: trying builtin");
	if (builtin_try(argc, argv))
	{
		LOG("builtin_process: builtin executed, drawing prompt");
		terminal_draw_prompt();
		return;
	}

	LOG("builtin_process: running external command");
	builtin_run_external(argv);

	LOG("builtin_process: external done, drawing prompt");
	terminal_draw_prompt();
}