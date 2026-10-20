#ifndef TL_CTX_H
#define TL_CTX_H

#include "main.h"
#include "tl_struct.h"

typedef struct tl_exec_ctx
{
    /* --- Execution Mode --- */
    bool posix_mode;    // strict POSIX semantics
    bool termlink_mode; // Fallout DSL extensions enabled
    bool interactive;   // interactive shell vs script

    /* --- Environment --- */
    char **envp;     // pointer to environment (for execve)
    int last_status; // $? last command exit status

    /* --- Working Directory --- */
    char cwd[PATH_MAX]; // cached current working directory

    /* --- Redirection State --- */
    int saved_fds[3]; // for restoring stdin/stdout/stderr

    /* --- Pager / Output Hooks --- */
    void (*output_fn)(const char *); // slow_print or pager-aware print
    void (*error_fn)(const char *);  // styled error output

    /* --- Temporary Assignment Environment --- */
    char **temp_env; // VAR=val before command execution

    /* --- Future: Functions, Aliases, Variables --- */
    // struct tl_function_table *functions;
    // struct tl_alias_table *aliases;
    // struct tl_var_table *vars;

} tl_exec_ctx_t;

extern tl_exec_ctx_t tl_ctx;

#endif
