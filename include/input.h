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
	CTRL_D,
	CTRL_U,
	CTRL_K,
	CTRL_Y,
	CTRL_A,
	CTRL_E,
	CTRL_L,
	CTRL_W,
	RESIZE = 2000,
};

void input_redraw(input_state *st, bool place_cursor);
void input_read_line(input_state *st, char *buf, size_t maxlen);
int input_read_char(void);
int input_parse_args(char *line, char **argv, int max);

#endif