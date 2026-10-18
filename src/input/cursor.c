#include "main.h"
#include "cursor.h"
#include "terminal.h"

void input_cursor_left(input_state *st)
{
	LOG("cursor_left: before cursor=%zu len=%zu buf='%s'",
		st->cursor, st->len, st->buf);

	if (st->cursor > 0)
		st->cursor--;

	LOG("cursor_left: after cursor=%zu", st->cursor);

	printf("\033[%d;%dH", terminal_rows, PROMPT_COL + (int)st->cursor);
	fflush(stdout);

	LOG("cursor_left: redraw complete");
}

void input_cursor_right(input_state *st)
{
	LOG("cursor_right: before cursor=%zu len=%zu buf='%s'",
		st->cursor, st->len, st->buf);

	if (st->cursor < st->len)
		st->cursor++;

	LOG("cursor_right: after cursor=%zu", st->cursor);

	printf("\033[%d;%dH", terminal_rows, PROMPT_COL + (int)st->cursor);
	fflush(stdout);

	LOG("cursor_right: redraw complete");
}

void input_cursor_redraw(const input_state *st)
{
	LOG("cursor_redraw: cursor=%zu len=%zu buf='%s'",
		st->cursor, st->len, st->buf);

	printf("\033[%d;%dH", terminal_rows, PROMPT_COL + (int)st->cursor);
	fflush(stdout);

	LOG("cursor_redraw: redraw complete");
}

void input_cursor_set(int row, int col)
{
	printf("\033[%d;%dH", row, col);
	fflush(stdout);
}

int input_cursor_get_col(void)
{
	printf("\033[6n"); // Request cursor position
	fflush(stdout);

	char buf[32];
	int idx = 0;

	// Read until we get 'R' or buffer fills
	while (idx < (int)sizeof(buf) - 1)
	{
		int c = getchar();
		if (c == EOF)
			break;

		buf[idx++] = (char)c;
		if (c == 'R')
			break;
	}

	buf[idx] = '\0';

	// Expected format: ESC [ row ; col R
	int row, col;
	if (sscanf(buf, "\033[%d;%dR", &row, &col) == 2)
		return col;

	return -1;
}