#include "main.h"
#include "input.h"
#include "terminal.h"
#include "tokens.h"
#include "read_char.h"
#include "buffer.h"
#include "cursor.h"
#include "autocomplete.h"

void input_read_line(input_state *st, char *buf, size_t maxlen)
{
	st->buf = buf;
	st->len = 0;
	st->cursor = 0;
	st->maxlen = maxlen;
	st->insert_mode = 1;
	st->tab_active = 0;
	st->killbuf[0] = '\0';
	st->kill_len = 0;

	terminal_draw_prompt();
	// printf("\033[%d;%dH", terminal_rows, PROMPT_COL);

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
		case BACKSPACE:
			input_buffer_backspace(st);
			input_redraw(st, true);
			break;

		case '\t':
			input_autocomplete_tab(st);
			st->cursor = st->len;
			input_redraw(st, true);
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

			input_redraw(st, false);
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

			input_redraw(st, false);
			break;

		case LEFT:
			input_cursor_left(st);
			input_redraw(st, true);
			break;

		case RIGHT:
			input_cursor_right(st);
			input_redraw(st, true);
			break;

		case DEL:
			input_buffer_delete(st);
			input_redraw(st, true);
			break;

		case CTRL_LEFT:
			st->cursor = input_tokens_prev(st->buf, st->cursor);
			input_redraw(st, true);
			break;

		case CTRL_RIGHT:
			st->cursor = input_tokens_next(st->buf, st->cursor);
			input_redraw(st, true);
			break;

		case CTRL_BACKSPACE:
		case ALT_BACKSPACE:
			input_tokens_delete_prev(st);
			input_redraw(st, true);
			break;

		case CTRL_DELETE:
		case ALT_D:
			input_tokens_delete_next(st);
			input_redraw(st, true);
			break;

		case CTRL_U:
			input_tokens_kill_to_begin(st);
			input_redraw(st, true);
			break;

		case CTRL_K:
			input_tokens_kill_to_end(st);
			input_redraw(st, true);
			break;

		case CTRL_Y:
			input_tokens_yank(st);
			input_redraw(st, true);
			break;

		default:
			if (ch >= 32 && ch < 127)
			{
				input_buffer_insert(st, (char)ch);
				input_redraw(st, true);
			}
			break;
		}
	}
}