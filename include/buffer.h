#ifndef BUFFER_H
#define BUFFER_H

#include "input_state.h"

void input_buffer_insert(input_state *st, char c);
void input_buffer_backspace(input_state *st);
void input_buffer_delete(input_state *st);

#endif