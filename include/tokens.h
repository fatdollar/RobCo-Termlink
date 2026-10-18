#ifndef TOKENS_H
#define TOKENS_H

#include "input_state.h"

int input_tokens_prev(const char *buf, int cursor);
int input_tokens_next(const char *buf, int cursor);

void input_tokens_delete_prev(input_state *st);
void input_tokens_delete_next(input_state *st);

void input_tokens_kill_to_begin(input_state *st);
void input_tokens_kill_to_end(input_state *st);

void input_tokens_yank(input_state *st);

#endif