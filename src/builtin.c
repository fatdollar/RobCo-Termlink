#include "main.h"
#include "builtin.h"
#include "terminal.h"
#include "util.h"
#include "history.h"

char builtin_external_cmds[MAX_EXTERNAL_CMDS][PATH_MAX];
int builtin_external_cmd_count = 0;

void builtin_register_external(const char *name) {
    if (builtin_external_cmd_count >= MAX_EXTERNAL_CMDS) return;
    snprintf(builtin_external_cmds[builtin_external_cmd_count],
             sizeof(builtin_external_cmds[builtin_external_cmd_count]),
             "%s", name);
    builtin_external_cmd_count++;
}

void builtin_load_commands_from_dir(const char *dirpath) {
    DIR *d = opendir(dirpath);
    if (!d) return;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        char fullpath[PATH_MAX];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dirpath, ent->d_name);

        if (util_is_executable(fullpath)) {
            builtin_register_external(ent->d_name);
        }
    }

    closedir(d);
}

void builtin_load_external_from_path(void) {
    const char *path = getenv("PATH");
    if (!path) return;

    char temp[4096];
    snprintf(temp, sizeof(temp), "%s", path);

    char *saveptr;
    char *dir = strtok_r(temp, ":", &saveptr);

    while (dir) {
        builtin_load_commands_from_dir(dir);
        dir = strtok_r(NULL, ":", &saveptr);
    }
}

void builtin_cd(int argc, char **argv) {
	const char *path = (argc > 1) ? argv[1] : getenv("HOME");
    if (!path) path = "/";
    if (chdir(path) != 0) {
        char buf[256];
        snprintf(buf, sizeof(buf), "cd: cannot change directory to %s", path);
        terminal_slow_print(buf);
    }
}

void builtin_exit(void) {
    terminal_slow_print("Shutting down terminal...");
    exit(0);
}

void builtin_help(void) {
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

    for (int i = 0; i < builtin_external_cmd_count; i++) {
        terminal_slow_print(builtin_external_cmds[i]);
    }
}

void builtin_export(int argc, char **argv) {
	if (argc < 2) {
        terminal_slow_print("export: usage: export NAME=VALUE");
        return;
    }
    char *eq = strchr(argv[1], '=');
    if (!eq) {
        terminal_slow_print("export: usage: export NAME=VALUE");
        return;
    }
    *eq = '\0';
    const char *name = argv[1];
    const char *value = eq + 1;
    if (setenv(name, value, 1) != 0) {
        terminal_slow_print("export: failed to set variable");
    }
}

void builtin_unset(int argc, char **argv) {
    if (argc < 2) {
        terminal_slow_print("unset: usage: unset NAME");
        return;
    }
    if (unsetenv(argv[1]) != 0) {
        terminal_slow_print("unset: failed to unset variable");
    }
}

int builtin_try(int argc, char **argv) {
    if (argc == 0) return 1;

    if (strcmp(argv[0], "cd") == 0) {
        builtin_cd(argc, argv);
        return 1;
    } else if (strcmp(argv[0], "exit") == 0) {
        builtin_exit();
        return 1;
    } else if (strcmp(argv[0], "help") == 0) {
        builtin_help();
        return 1;
    } else if (strcmp(argv[0], "clear") == 0) {
        terminal_clear_scroll();
        return 1;
    } else if (strcmp(argv[0], "export") == 0) {
        builtin_export(argc, argv);
        return 1;
    } else if (strcmp(argv[0], "unset") == 0) {
        builtin_unset(argc, argv);
        return 1;
    }

    return 0;
}

void builtin_run_external(char **argv) {
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        terminal_slow_print("Error: pipe failed");
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        terminal_slow_print("Error: fork failed");
        return;
    }

    if (pid == 0) {
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

    while ((n = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[n] = '\0';
        terminal_slow_print(buffer);
    }

    close(pipefd[0]);

    int status;
    waitpid(pid, &status, 0);
}

void builtin_process(const char *cmd) {
    char buf[PATH_MAX];
    char *argv[MAX_ARGS];
    int argc;

    snprintf(buf, sizeof(buf), "%s", cmd);

    argc = input_parse_args(buf, argv, MAX_ARGS);
    if (argc == 0) return;

    // always clear before executing a command
    terminal_clear_scroll();
	history_add(cmd);
    terminal_cursor_line = terminal_scroll_top;

    if (builtin_try(argc, argv)) {
        terminal_draw_prompt();
        return;
    }

    builtin_run_external(argv);
    terminal_draw_prompt();
}