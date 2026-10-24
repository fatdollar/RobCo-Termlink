#include "main.h"
#include "input.h"
#include "terminal.h"
#include "tokens.h"
#include "buffer.h"
#include "cursor.h"
#include "autocomplete.h"

void input_read_line(input_state *st, char *buf, size_t maxlen)
{
	LOG("read_line: start maxlen=%zu", maxlen);

	st->buf = buf;
	st->len = 0;
	st->cursor = 0;
	st->maxlen = maxlen;
	st->insert_mode = 1;
	st->tab_active = 0;
	st->killbuf[0] = '\0';
	st->kill_len = 0;
	memset(st->buf, 0, st->maxlen);

	LOG("read_line: state initialized");

	while (1)
	{
		/* Check if the user resized the window while typing or waiting */
		if (terminal_check_resize())
		{
			LOG("read_line: window resize detected, redrawing UI layout");

			// 1. Wipe and re-render the RobCo header to the new width
			terminal_header_print();
			terminal_prompt_draw();

			// 2. Clear any old scroll lines/trash and redraw input prompt at new terminal_rows
			input_redraw(st, true);
		}

		int ch = input_read_char();
		LOG("read_line: got ch=%d", ch);

		/* Optional: Handle EINTR from SIGWINCH if getchar was interrupted */
		if (ch == EOF && errno == EINTR)
		{
			if (terminal_check_resize())
			{
				terminal_header_print();
				input_redraw(st, true);
			}
			continue;
		}

		switch (ch)
		{
		case '\n':
		case '\r':
			LOG("read_line: newline");
			st->buf[st->len] = '\0';
			LOG("read_line: returning line='%s'", st->buf);
			return;

		case 127:
		case '\b':
		case BACKSPACE:
			LOG("read_line: BACKSPACE");
			input_buffer_backspace(st);
			input_redraw(st, true);
			break;

		case '\t':
			LOG("read_line: TAB");
			input_autocomplete_tab(st);
			st->cursor = st->len;
			input_redraw(st, true);
			break;

		case UP:
			LOG("read_line: UP");
			if (st->hist.history_count == 0)
			{
				LOG("read_line: history empty");
				buf[0] = '\0';
				st->len = 0;
				st->cursor = 0;
				input_redraw(st, false);
				break;
			}

			history_prev(&st->hist, st->buf, st->maxlen);
			st->len = strlen(st->buf);
			st->cursor = st->len;
			st->buf[st->len] = '\0';

			LOG("read_line: history_prev result='%s'", st->buf);
			input_redraw(st, false);
			break;

		case DOWN:
			LOG("read_line: DOWN");
			if (st->hist.history_count == 0)
			{
				LOG("read_line: history empty");
				buf[0] = '\0';
				st->len = 0;
				st->cursor = 0;
				input_redraw(st, false);
				break;
			}

			history_next(&st->hist, st->buf, st->maxlen);
			st->len = strlen(st->buf);
			st->cursor = st->len;
			st->buf[st->len] = '\0';

			LOG("read_line: history_next result='%s'", st->buf);
			input_redraw(st, false);
			break;

		case LEFT:
			LOG("read_line: LEFT");
			input_cursor_left(st); // cursor.c already moves cursor
			break;				   // NO full redraw

		case RIGHT:
			LOG("read_line: RIGHT");
			input_cursor_right(st); // cursor.c already moves cursor
			break;					// NO full redraw

		case DEL:
			LOG("read_line: DEL");
			input_buffer_delete(st);
			input_redraw(st, true);
			break;

		case CTRL_LEFT:
			LOG("read_line: CTRL_LEFT");
			st->cursor = input_tokens_prev(st->buf, st->cursor);
			input_cursor_redraw(st); // lightweight cursor move
			break;

		case CTRL_RIGHT:
			LOG("read_line: CTRL_RIGHT");
			st->cursor = input_tokens_next(st->buf, st->cursor);
			input_cursor_redraw(st); // lightweight cursor move
			break;

		case CTRL_BACKSPACE:
		case ALT_BACKSPACE:
			LOG("read_line: CTRL/ALT_BACKSPACE");
			input_tokens_delete_prev(st);
			input_redraw(st, true);
			break;

		case CTRL_DELETE:
		case ALT_D:
			LOG("read_line: CTRL_DELETE / ALT_D");
			input_tokens_delete_next(st);
			input_redraw(st, true);
			break;

		case CTRL_C:
			st->len = 0;
			st->cursor = 0;
			st->buf[0] = '\0';

			terminal_slow_print("\n");
			input_redraw(st, false);
			continue;

		case CTRL_U:
			LOG("read_line: CTRL_U");
			input_tokens_kill_to_begin(st);
			input_redraw(st, true);
			break;

		case CTRL_K:
			LOG("read_line: CTRL_K");
			input_tokens_kill_to_end(st);
			input_redraw(st, true);
			break;

		case CTRL_Y:
			LOG("read_line: CTRL_Y");
			input_tokens_yank(st);
			input_redraw(st, true);
			break;

		case CTRL_A:
			LOG("read_line: CTRL_A (Beginning of line)");
			st->cursor = 0;
			input_cursor_redraw(st);
			break;

		case CTRL_E:
			LOG("read_line: CTRL_E (End of line)");
			st->cursor = st->len;
			input_cursor_redraw(st);
			break;

		case CTRL_D:
			LOG("read_line: CTRL_D");
			if (st->len == 0)
			{
				// Empty line EOF -> exit shell or signal EOF
				LOG("read_line: EOF received on empty line");
				terminal_slow_print_at(terminal_rows, "Shutting down...\n");
				exit(0);
			}
			else
			{
				// Delete character under cursor (like DEL)
				LOG("read_line: deleting char under cursor");
				input_buffer_delete(st);
				input_redraw(st, true);
			}
			break;

		case CTRL_L:
			LOG("read_line: CTRL_L (Clear screen)");
			terminal_scroll_clear();
			input_redraw(st, true);
			break;

		case CTRL_W:
			LOG("read_line: CTRL_W (Kill word backward)");
			input_tokens_delete_prev(st);
			input_redraw(st, true);
			break;

		case RESIZE:
			terminal_header_print();
			input_redraw(st, true);
			break;

		default:
			if (ch >= 32 && ch < 127)
			{
				LOG("read_line: insert '%c'", ch);
				input_buffer_insert(st, (char)ch);
				input_redraw(st, true);
			}
			else
			{
				LOG("read_line: ignored ch=%d", ch);
			}
			break;
		}
	}
}