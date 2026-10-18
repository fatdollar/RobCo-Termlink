#ifndef CURSOR_H
#define CURSOR_H

#include "input_state.h"

void input_cursor_left(input_state *st);
void input_cursor_right(input_state *st);
void input_cursor_redraw(const input_state *st);
void input_cursor_set(int row, int col);
int input_cursor_get_col(void);

#endif