#include "main.h"
#include "buffer.h"
#include "terminal.h"


void input_buffer_insert(input_state *st, char c)
{
    if (st->len >= st->maxlen - 1)
        return;

    /* Shift right from cursor */
    memmove(st->buf + st->cursor + 1,
            st->buf + st->cursor,
            st->len - st->cursor);

    st->buf[st->cursor] = c;
    st->cursor++;
    st->len++;

    /* Redraw entire line */
    printf("\033[%d;%dH", terminal_rows, PROMPT_COL);
    for (size_t i = 0; i < st->len; i++)
        terminal_slow_putc(st->buf[i]);

    /* Move cursor to correct position */
    printf("\033[%d;%dH", terminal_rows, PROMPT_COL + (int)st->cursor);
    fflush(stdout);
}

void input_buffer_backspace(input_state *st)
{
    if (st->cursor == 0)
        return;

    /* Shift left */
    memmove(st->buf + st->cursor - 1,
            st->buf + st->cursor,
            st->len - st->cursor);

    st->cursor--;
    st->len--;
    st->buf[st->len] = '\0';

    /* Redraw */
    printf("\033[%d;%dH", terminal_rows, PROMPT_COL);
    for (size_t i = 0; i < st->len; i++)
        terminal_slow_putc(st->buf[i]);

    /* Clear trailing char */
    terminal_slow_putc(' ');

    /* Restore cursor */
    printf("\033[%d;%dH", terminal_rows, PROMPT_COL + (int)st->cursor);
    fflush(stdout);
}

void input_buffer_delete(input_state *st)
{
    if (st->cursor >= st->len)
        return;

    memmove(st->buf + st->cursor,
            st->buf + st->cursor + 1,
            st->len - st->cursor - 1);

    st->len--;
    st->buf[st->len] = '\0';

    /* Redraw */
    printf("\033[%d;%dH", terminal_rows, PROMPT_COL);
    for (size_t i = 0; i < st->len; i++)
        terminal_slow_putc(st->buf[i]);

    terminal_slow_putc(' ');

    printf("\033[%d;%dH", terminal_rows, PROMPT_COL + (int)st->cursor);
    fflush(stdout);
}