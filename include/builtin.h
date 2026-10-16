#ifndef BUILTIN_H
#define BUILTIN_H

#define MAX_EXTERNAL_CMDS 1024

extern char builtin_external_cmds[][PATH_MAX];
extern int builtin_external_cmd_count;

void builtin_register_external(const char *name);
void builtin_load_commands_from_dir(const char *dirpath);
void builtin_load_external_from_path(void);

#ifdef TL_CD
int builtin_cd(int argc, char **argv);
#endif
#ifdef TL_EXIT
int builtin_exit(int argc, char **argv);
#endif
int builtin_help(int argc, char **argv);
#ifdef TL_CLEAR
int builtin_clear(int argc, char **argv);
#endif
#ifdef TL_EXPORT
int builtin_export(int argc, char **argv);
#endif
#ifdef TL_UNSET
int builtin_unset(int argc, char **argv);
#endif

int builtin_try(int argc, char **argv);
void builtin_run_external(char **argv);
void builtin_process(const char *cmd);

#endif