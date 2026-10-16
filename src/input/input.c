#include "main.h"
#include "input.h"
#include "input_state.h"
#include "cursor.h"
#include "buffer.h"
#include "autocomplete.h"
#include "history.h"
#include "terminal.h"

static int input_read_char(void)
{
	int c = getchar();

	if (c != 27)
		return c;

	int c1 = getchar();
	if (c1 != '[')
	{
		ungetc(c1, stdin);
		return 27;
	}

	int c2 = getchar();

	// Standard arrows
	if (c2 == 'A')
		return UP;
	if (c2 == 'B')
		return DOWN;
	if (c2 == 'C')
		return RIGHT;
	if (c2 == 'D')
		return LEFT;

	// Delete
	if (c2 == '3')
	{
		int c3 = getchar();
		if (c3 == '~')
			return DEL;
	}

	// CTRL+Left / CTRL+Right: ESC [ 1 ; 5 D / C
	if (c2 == '1')
	{
		int c3 = getchar(); // should be ';'
		int c4 = getchar(); // should be '5'
		int c5 = getchar(); // D or C

		if (c3 == ';' && c4 == '5')
		{
			if (c5 == 'D')
				return CTRL_LEFT;
			if (c5 == 'C')
				return CTRL_RIGHT;
		}
	}

	// fallback
	ungetc(c2, stdin);
	ungetc('[', stdin);
	return 27;
}

int input_parse_args(char *line, char **argv, int max_args)
{
	int argc = 0;
	char *p = line;

	while (*p && argc < max_args - 1)
	{
		while (*p == ' ' || *p == '\t')
			p++;
		if (!*p)
			break;

		argv[argc++] = p;

		while (*p && *p != ' ' && *p != '\t')
			p++;
		if (*p)
		{
			*p = '\0';
			p++;
		}
	}

	argv[argc] = NULL;
	return argc;
}

void input_read_line(input_state *st, char *buf, size_t maxlen)
{
	st->buf = buf;
	st->len = 0;
	st->cursor = 0;
	st->maxlen = maxlen;
	st->insert_mode = 1;
	st->tab_active = 0;

	terminal_draw_prompt();
	printf("\033[%d;%dH", terminal_rows, PROMPT_COL);

	while (1)
	{
		int ch = input_read_char();

		switch (ch)
		{

		case '\n':
		case '\r':
			st->buf[st->len] = '\0';
			terminal_draw_prompt();
			return;

		case 127:
		case '\b':
			input_buffer_backspace(st);
			break;

		case '\t':
			input_autocomplete_tab(st);
			printf("\033[%d;%dH", terminal_rows, PROMPT_COL);
			for (size_t i = 0; i < st->len; i++)
				terminal_slow_putc(st->buf[i]);
			printf("\033[%d;%dH", terminal_rows, PROMPT_COL + (int)st->len);
			fflush(stdout);
			break;

		case UP:
			if (st->hist.history_count == 0)
			{
				buf[0] = '\0';
				break;
			}

			history_prev(&st->hist, st->buf, st->maxlen);
			st->len = strlen(st->buf);
			st->cursor = st->len;

			terminal_draw_prompt();
			printf("\033[%d;%dH", terminal_rows, PROMPT_COL);
			printf("\033[K");
			printf("%s", st->buf);
			fflush(stdout);
			break;

		case DOWN:
			if (st->hist.history_count == 0)
			{
				buf[0] = '\0';
				break;
			}

			history_next(&st->hist, st->buf, st->maxlen);
			st->len = strlen(st->buf);
			st->cursor = st->len;

			terminal_draw_prompt();
			printf("\033[%d;%dH", terminal_rows, PROMPT_COL);
			printf("\033[K");
			printf("%s", st->buf);
			fflush(stdout);
			break;

		case LEFT:
			input_cursor_left(st);
			break;

		case RIGHT:
			input_cursor_right(st);
			break;

		case DEL:
			input_buffer_delete(st);
			break;

		default:
			if (ch >= 32 && ch < 127)
			{
				input_buffer_insert(st, (char)ch);
			}
			break;
		}
	}
}