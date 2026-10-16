#ifndef INPUT_H
#define INPUT_H

#include "input_state.h"

#define MAX_ARGS 64

enum
{
	UP = 1,
	DOWN,
	LEFT,
	RIGHT,
	DEL,
	CTRL_LEFT,
	CTRL_RIGHT,
};

int input_parse_args(char *line, char **argv, int max_args);
void input_read_line(input_state *st, char *buf, size_t maxlen);

#endif