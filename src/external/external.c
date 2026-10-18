#include "main.h"
#include "external.h"
#include "terminal.h"
#include "pager.h"
#include "input.h"

/* ============================================================
 * GLOBAL STATE
 * ============================================================ */

char external_cmds[MAX_EXTERNAL_CMDS][PATH_MAX];
int external_cmd_count = 0;

/* ============================================================
 * INTERNAL HELPERS (static)
 * ============================================================ */

static void _external_register(const char *name)
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

static int _external_is_executable(const char *path)
{
    LOG("_external_is_executable: path='%s'", path);

    struct stat st;
    if (stat(path, &st) != 0)
    {
        LOG("_external_is_executable: stat failed");
        return 0;
    }

    if (!S_ISREG(st.st_mode))
    {
        LOG("_external_is_executable: not a regular file");
        return 0;
    }

    int exec = (st.st_mode & S_IXUSR) ||
               (st.st_mode & S_IXGRP) ||
               (st.st_mode & S_IXOTH);

    LOG("_external_is_executable: exec=%d", exec);
    return exec;
}

static void _external_load_commands_from_dir(const char *dirpath)
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

        if (_external_is_executable(fullpath))
        {
            LOG("load_commands_from_dir: executable '%s'", ent->d_name);
            _external_register(ent->d_name);
        }
    }

    closedir(d);
}

/* ============================================================
 * PUBLIC API
 * ============================================================ */

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
        _external_load_commands_from_dir(dir);
        dir = strtok_r(NULL, ":", &saveptr);
    }
#endif
}

bool external_exists(const char *cmd)
{
    LOG("external_exists: checking '%s'", cmd);

    const char *path = getenv("PATH");
    if (!path)
    {
        LOG("external_exists: PATH is null");
        return false;
    }

    LOG("external_exists: PATH='%s'", path);

    char buf[PATH_MAX];
    const char *p = path;

    while (*p)
    {
        const char *start = p;
        while (*p && *p != ':')
            p++;

        size_t len = p - start;

        if (len > 0)
        {
            snprintf(buf, sizeof(buf), "%.*s/%s", (int)len, start, cmd);
            LOG("external_exists: checking file '%s'", buf);

            if (access(buf, X_OK) == 0)
            {
                LOG("external_exists: found executable '%s'", buf);
                return true;
            }
        }

        if (*p == ':')
            p++;
    }

    LOG("external_exists: '%s' not found", cmd);
    return false;
}

bool external_is_tty_interactive(const char *cmd)
{
    LOG("external_is_tty_interactive: checking '%s'", cmd);

    const char *interactive[] = {
        "bash", "sh", "zsh", "fish",
        "nano", "vim", "vi",
        "top", "htop",
        "less", "more",
        "ssh",
        "python", "python3",
        NULL};

    for (int i = 0; interactive[i]; i++)
    {
        if (strcmp(cmd, interactive[i]) == 0)
        {
            LOG("external_is_tty_interactive: '%s' is known interactive", cmd);
            return true;
        }
    }

    LOG("external_is_tty_interactive: '%s' not in interactive list", cmd);
    return false;
}

void external_run(char **argv)
{
    LOG("run_external: cmd='%s'", argv[0]);

#ifndef TL_EXTERNAL_CMDS
    terminal_slow_print("External commands are disabled.\n");
    return;
#endif

    int pipefd[2];
    if (pipe(pipefd) < 0)
    {
        terminal_slow_print("Error: pipe failed\n");
        return;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        terminal_slow_print("Error: fork failed\n");
        return;
    }

    if (pid == 0)
    {
        if (termlink_job_control_enabled)
            setpgid(0, 0);

        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = SIG_DFL;
        sigaction(SIGINT, &sa, NULL);

        close(pipefd[0]);

        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);

        execvp(argv[0], argv);
        perror("execvp");
        _exit(127);
    }

    close(pipefd[1]);

    if (termlink_job_control_enabled)
    {
        setpgid(pid, pid);
        tcsetpgrp(STDIN_FILENO, pid);
    }

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

    if (termlink_job_control_enabled)
        tcsetpgrp(STDIN_FILENO, getpgrp());

    LOG("run_external: child exited status=%d", status);
}

void external_run_paged(char **argv)
{
    int pipefd[2];
    pipe(pipefd);

    pid_t pid = fork();
    if (pid == 0)
    {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        execvp(argv[0], argv);
        _exit(127);
    }

    close(pipefd[1]);

    pager p;
    pager_init(&p, terminal_scroll_rows(), terminal_scroll_cols());

    char buf[4096];
    ssize_t n;

    while ((n = read(pipefd[0], buf, sizeof(buf))) > 0)
    {
        pager_feed(&p, buf, (size_t)n);

        if (p.quit)
            break;

        pager_run(&p);

        if (p.quit)
            break;
    }

    if (!p.quit)
    {
        p.done = true;

        terminal_scroll_clear();
        pager_render(&p);

        int key;
        if (!terminal_poll_key(&key))
            key = input_read_char();
        pager_handle_key(&p, key);

        if (p.quit)
        {
            terminal_scroll_clear();
            terminal_prompt_draw();
        }
    }

    pager_free(&p);
    close(pipefd[0]);
    waitpid(pid, NULL, 0);
}