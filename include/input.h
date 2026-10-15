#ifndef INPUT_H
#define INPUT_H

#include <stddef.h>
#include <limits.h>

#define MAX_COMPLETIONS 128
#define MAX_ARGS 64

#define KEY_UP    1001
#define KEY_DOWN  1002

typedef enum {
    COMPLETE_FILES_AND_DIRS = 0,
    COMPLETE_DIRS,
    COMPLETE_FILES
} completion_type;

typedef struct {
    char cmd[64];
    completion_type type;
} completion_entry;

extern completion_entry input_completion_table[];
extern int input_completion_count;

int input_parse_args(char *line, char **argv, int max_args);
completion_entry *input_lookup_completion(const char *cmd);

void input_complete_path(char **argv, size_t *len, char *buf, completion_type type);
void input_handle_tab(char *buf, size_t *len);
void input_read_line(char *buf, size_t maxlen);

#endif