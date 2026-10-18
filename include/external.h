#ifndef EXTERNAL_H
#define EXTERNAL_H

#define MAX_EXTERNAL_CMDS 256

extern char external_cmds[MAX_EXTERNAL_CMDS][PATH_MAX];
extern int external_cmd_count;

void external_register(const char *name);
void external_load_from_path(void);
void external_load_commands_from_dir(const char *dirpath);
bool external_exists(const char *cmd);
bool external_is_tty_interactive(const char *cmd);
void external_run(char **argv);
void external_run_paged(char **argv);

#endif