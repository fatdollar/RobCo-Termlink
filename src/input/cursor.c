#include "main.h"
#include "cursor.h"
#include "terminal.h"

void input_cursor_left(input_state *st)
{
    if (st->cursor > 0)
        st->cursor--;

    printf("\033[%d;%dH", terminal_rows, PROMPT_COL + (int)st->cursor);
    fflush(stdout);
}

void input_cursor_right(input_state *st)
{
    if (st->cursor < st->len)
        st->cursor++;

    printf("\033[%d;%dH", terminal_rows, PROMPT_COL + (int)st->cursor);
    fflush(stdout);
}

void input_cursor_redraw(const input_state *st)
{
    printf("\033[%d;%dH", terminal_rows, PROMPT_COL + (int)st->cursor);
    fflush(stdout);
}