#include "main.h"
#include "util.h"
#include "input.h"
#include "autocomplete.h"

int util_is_executable(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    if (!S_ISREG(st.st_mode)) return 0;
    return (st.st_mode & S_IXUSR) ||
           (st.st_mode & S_IXGRP) ||
           (st.st_mode & S_IXOTH);
}

autocomplete_type util_parse_completion_type(const char *value) {
    if (strcmp(value, "dirs") == 0) return AUTOCOMPLETE_DIRS;
    if (strcmp(value, "files") == 0) return AUTOCOMPLETE_FILES;
    return AUTOCOMPLETE_FILES_AND_DIRS;
}

void util_register_completion(const char *cmd, autocomplete_type type) {
    if (input_autocomplete_count >= MAX_COMPLETIONS) return;
    snprintf(input_autocomplete_table[input_autocomplete_count].cmd,
             sizeof(input_autocomplete_table[input_autocomplete_count].cmd),
             "%s", cmd);
    input_autocomplete_table[input_autocomplete_count].type = type;
    input_autocomplete_count++;
}

void util_load_completions_dir(const char *dirpath) {
    DIR *d = opendir(dirpath);
    if (!d) return;

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        const char *name = ent->d_name;
        size_t len = strlen(name);
        if (len <= 5) continue;
        if (strcmp(name + len - 5, ".comp") != 0) continue;

        char cmd[64];
        size_t cmdlen = len - 5;
        if (cmdlen >= sizeof(cmd)) cmdlen = sizeof(cmd) - 1;
        memcpy(cmd, name, cmdlen);
        cmd[cmdlen] = '\0';

        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dirpath, name);

        FILE *f = fopen(path, "r");
        if (!f) continue;

        char line[256];
        autocomplete_type type = AUTOCOMPLETE_FILES_AND_DIRS;
        while (fgets(line, sizeof(line), f)) {
            char *nl = strchr(line, '\n');
            if (nl) *nl = '\0';
            if (strncmp(line, "type=", 5) == 0) {
                const char *val = line + 5;
                type = util_parse_completion_type(val);
                break;
            }
        }
        fclose(f);

        util_register_completion(cmd, type);
    }

    closedir(d);
}

void util_load_completions(void) {
    util_load_completions_dir("/usr/share/termlink/completions");
    util_load_completions_dir("/etc/termlink/completions");

    const char *home = getenv("HOME");
    if (home && *home) {
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/.termlink/completions", home);
        util_load_completions_dir(path);
    }

    /* built-in defaults if no files exist */
    util_register_completion("cd", AUTOCOMPLETE_DIRS);
}