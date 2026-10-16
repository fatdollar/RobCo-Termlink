#include "buffer.h"
#include "terminal.h"
#include <stdio.h>

void input_buffer_insert(input_state *st, char c)
{
    if (st->len + 1 < st->maxlen) {
        st->buf[st->len++] = c;

        printf("\033[%d;%dH", terminal_rows, PROMPT_COL + (int)(st->len - 1));
        terminal_slow_putc(c);
        fflush(stdout);
    }
}

void input_buffer_backspace(input_state *st)
{
    if (st->len > 0) {
        st->len--;

        printf("\033[%d;%dH", terminal_rows, PROMPT_COL + (int)st->len);
        terminal_slow_putc(' ');
        printf("\033[%d;%dH", terminal_rows, PROMPT_COL + (int)st->len);
        fflush(stdout);
    }
}

void input_buffer_delete(input_state *st)
{
    /* No delete key yet — Phase 4 will implement this */
}