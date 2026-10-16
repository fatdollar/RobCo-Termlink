#ifndef AUTOCOMPLETE_H
#define AUTOCOMPLETE_H

#include <stddef.h>
#include <limits.h>

#define MAX_COMPLETIONS 128

typedef enum {
    AUTOCOMPLETE_FILES_AND_DIRS = 0,
    AUTOCOMPLETE_DIRS,
    AUTOCOMPLETE_FILES
} autocomplete_type;

typedef struct {
    char cmd[64];
    autocomplete_type type;
} autocomplete_entry;

extern autocomplete_entry input_autocomplete_table[];
extern int input_autocomplete_count;

void input_autocomplete_path(char **argv, size_t *len, char *buf, autocomplete_type type);
void input_autocomplete_tab(char *buf, size_t *len);
autocomplete_entry *input_autocomplete_lookup(const char *cmd);

#endif