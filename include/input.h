#ifndef INPUT_H
#define INPUT_H

#include "input_state.h"

#define MAX_ARGS 64

enum
{
	UP = 1000,
	DOWN,
	LEFT,
	RIGHT,
	DEL,
	BACKSPACE,
	CTRL_LEFT,
	CTRL_RIGHT,
	CTRL_BACKSPACE,
	CTRL_DELETE,
	ALT_BACKSPACE,
	ALT_D,
	CTRL_C,
	CTRL_U,
	CTRL_K,
	CTRL_Y,
};

void input_redraw(input_state *st, bool place_cursor);
int input_parse_args(char *line, char **argv, int max_args);
void input_read_line(input_state *st, char *buf, size_t maxlen);

#endif