#ifndef INPUT_H
#define INPUT_H

#include <stddef.h>
#include <limits.h>

#define MAX_ARGS 64

#define KEY_UP    1001
#define KEY_DOWN  1002

int input_parse_args(char *line, char **argv, int max_args);
void input_read_line(char *buf, size_t maxlen);

#endif