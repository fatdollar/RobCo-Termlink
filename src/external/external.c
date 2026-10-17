#include "main.h"
#include "external.h"
#include "terminal.h"
#include "util.h"

char external_cmds[MAX_EXTERNAL_CMDS][PATH_MAX];
int external_cmd_count = 0;

void external_register(const char *name)
{
	LOG("register_external: '%s'", name);

	if (external_cmd_count >= MAX_EXTERNAL_CMDS)
	{
		LOG("register_external: MAX_EXTERNAL_CMDS reached");
		return;
	}

	snprintf(external_cmds[external_cmd_count],
			 sizeof(external_cmds[external_cmd_count]),
			 "%s", name);

	external_cmd_count++;

	LOG("register_external: count=%d", external_cmd_count);
}

void external_load_from_path(void)
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

void external_load_commands_from_dir(const char *dirpath)
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
            external_register(ent->d_name);
        }
    }

    closedir(d);
}

void external_run(char **argv)
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