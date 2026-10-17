#ifndef BUILTIN_H
#define BUILTIN_H

#define MAX_BUILTINS 256

typedef int (*builtin_func)(int argc, char **argv);

typedef struct
{
    const char *name;
    builtin_func func;
} builtin_entry;

/* builtin registry */
int builtin_try(int argc, char **argv);
void builtin_init(void);
void builtin_register(const char *name, builtin_func func);

/* ============================================================
 * POSIX REQUIRED BUILTINS
 * ============================================================ */
int builtin_cd(int argc, char **argv);
int builtin_echo(int argc, char **argv);
int builtin_exit(int argc, char **argv);
int builtin_export(int argc, char **argv);
int builtin_unset(int argc, char **argv);
int builtin_pwd(int argc, char **argv);
int builtin_type(int argc, char **argv);
int builtin_set(int argc, char **argv);
int builtin_shift(int argc, char **argv);
int builtin_read(int argc, char **argv);
int builtin_umask(int argc, char **argv);
int builtin_alias(int argc, char **argv);
int builtin_unalias(int argc, char **argv);
int builtin_source(int argc, char **argv); /* '.' command */
int builtin_trap(int argc, char **argv);
int builtin_getopts(int argc, char **argv);

/* ============================================================
 * NON‑POSIX BUILTINS (TERMLINK CUSTOM)
 * ============================================================ */
int builtin_clear(int argc, char **argv);
int builtin_help(int argc, char **argv);

/* ============================================================
 * POSIX JOB CONTROL BUILTINS
 * ============================================================ */
int builtin_jobs(int argc, char **argv);
int builtin_fg(int argc, char **argv);
int builtin_bg(int argc, char **argv);
int builtin_kill(int argc, char **argv);
int builtin_wait(int argc, char **argv);

#endif