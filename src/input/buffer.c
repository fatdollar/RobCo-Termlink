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

	/* Move to prompt line + cursor */
	printf("\033[%d;%dH", terminal_rows, PROMPT_COL + (int)(st->cursor - 1));

	/* Print inserted char */
	putchar(c);

	/* Print the rest of the buffer (the shifted part) */
	for (size_t i = st->cursor; i < st->len; i++)
		putchar(st->buf[i]);

	/* Clear trailing char */
	putchar(' ');

	/* Restore cursor */
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
		putchar(st->buf[i]);

	/* Clear trailing char */
	putchar(' ');

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
		putchar(st->buf[i]);

	putchar(' ');

	printf("\033[%d;%dH", terminal_rows, PROMPT_COL + (int)st->cursor);
	fflush(stdout);
}