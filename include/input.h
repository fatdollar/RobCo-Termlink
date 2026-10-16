#ifndef INPUT_H
#define INPUT_H

#define MAX_ARGS 64

#define KEY_UP    1001
#define KEY_DOWN  1002
#define KEY_LEFT  1003
#define KEY_RIGHT 1004
#define KEY_DEL   1005

int input_parse_args(char *line, char **argv, int max_args);
void input_read_line(char *buf, size_t maxlen);

#endif