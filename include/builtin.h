#ifndef BUILTIN_H
#define BUILTIN_H

#include <limits.h>

#define MAX_EXTERNAL_CMDS 1024

extern char builtin_external_cmds[][PATH_MAX];
extern int builtin_external_cmd_count;

void builtin_register_external(const char *name);
void builtin_load_commands_from_dir(const char *dirpath);
void builtin_load_external_from_path(void);

void builtin_cd(int argc, char **argv);
void builtin_exit(void);
void builtin_help(void);
void builtin_clear(void);
void builtin_export(int argc, char **argv);
void builtin_unset(int argc, char **argv);

int builtin_try(int argc, char **argv);
void builtin_run_external(char **argv);
void builtin_process(const char *cmd);

#endif