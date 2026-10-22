#include "main.h"
#include "pager.h"
#include "terminal.h"
#include "input.h"

/* ============================================================
 * FORWARD DECLARATIONS
 * ============================================================ */

static void _screen_pager_buf_push(pager_buffer *b, const char *s, size_t n);
static void _screen_pager_columns_init(pager_column *pc, char matches[][PATH_MAX], int count);
static void _screen_pager_columns_build(pager_column *pc, char matches[][PATH_MAX], int count, pager *p);
static void _screen_pager_get_input(pager *p);

/* ============================================================
 *  INTERNAL HELPERS
 * ============================================================ */

static void _screen_pager_buf_push(pager_buffer *b, const char *s, size_t n)
{
	if (b->count == b->cap)
	{
		b->cap = b->cap ? b->cap * 2 : 64;
		b->lines = realloc(b->lines, b->cap * sizeof(pager_line));
	}
	b->lines[b->count].text = malloc(n + 1);
	memcpy(b->lines[b->count].text, s, n);
	b->lines[b->count].text[n] = 0;
	b->lines[b->count].len = n;
	b->count++;
}

static void _screen_pager_columns_init(pager_column *pc, char matches[][PATH_MAX], int count)
{
	pc->term_width = terminal_cols;
	pc->col_width = 0;

	/* Find longest entry */
	for (int i = 0; i < count; i++)
	{
		int len = strlen(matches[i]);
		if (len > pc->col_width)
			pc->col_width = len;
	}

	/* Add spacing */
	pc->col_width += 2;

	/* Compute number of columns */
	pc->cols = pc->term_width / pc->col_width;
	if (pc->cols < 1)
		pc->cols = 1;
}

static void _screen_pager_columns_build(pager_column *pc, char matches[][PATH_MAX], int count, pager *p)
{
	int rows = (count + pc->cols - 1) / pc->cols;

	for (int r = 0; r < rows; r++)
	{
		char line[PATH_MAX * 2] = {0};
		char *out = line;

		for (int c = 0; c < pc->cols; c++)
		{
			int idx = r + c * rows;
			if (idx >= count)
				break;

			const char *m = matches[idx];
			int len = strlen(m);

			memcpy(out, m, len);
			out += len;

			/* pad */
			int pad = pc->col_width - len;
			memset(out, ' ', pad);
			out += pad;
		}

		*out = '\0';
		pager_print_line(p, line);
	}
}

static void _screen_pager_get_input(pager *p)
{
	while (1)
	{
		int key;

		if (!terminal_poll_key(&key))
			key = input_read_char(); // BLOCK

		int old_top = p->top;
		screen_pager_handle_key(p, key);

		if (p->screen.quit)
		{
			terminal_scroll_clear();
			terminal_prompt_draw();
			break;
		}

		if (p->top != old_top)
		{
			terminal_scroll_clear();
			screen_pager_render(p);
		}
	}
}

/* ============================================================
 *  SETUP
 * ============================================================ */

void screen_pager_init(pager *p, int rows, int cols)
{
	memset(p, 0, sizeof(*p));
	p->rows = rows;
	p->cols = cols;
	p->cur_cap = 256;
	p->cur = malloc(p->cur_cap);

	p->screen.type = SCREEN_PAGER;
	p->screen.render = screen_pager_render;
	p->screen.handle_key = screen_pager_handle_key;
	p->screen.destroy = screen_pager_free;
}

/* ============================================================
 *  PROCESSING
 * ============================================================ */

void screen_pager_start(pager *p)
{
	screen_pager_init(p, terminal_scroll_rows(), terminal_cols);
}

void screen_pager_feed(pager *p, const char *data, size_t n)
{
	for (size_t i = 0; i < n; i++)
	{
		char c = data[i];
		if (c == '\n')
		{
			_screen_pager_buf_push(&p->buf, p->cur, p->cur_len);
			p->cur_len = 0;
		}
		else
		{
			if (p->cur_len + 1 >= p->cur_cap)
			{
				p->cur_cap *= 2;
				p->cur = realloc(p->cur, p->cur_cap);
			}
			p->cur[p->cur_len++] = c;
		}
	}
}

void pager_print_line(pager *p, const char *s)
{
	screen_pager_feed(p, s, strlen(s));
	screen_pager_feed(p, "\n", 1);
}

void screen_pager_finish(pager *p)
{
	p->done = true;
}

void screen_pager_run(pager *p)
{
	// Clear scroll region
	terminal_scroll_clear();

	// Render first page
	LOG("pager rows=%d buf.count=%zu top=%d", p->rows, p->buf.count, p->top);
	screen_pager_render(p);

	_screen_pager_get_input(p);
}

void screen_pager_render(pager *p)
{
	int first = p->top;
	int last = first + p->rows;
	if (last > (int)p->buf.count)
		last = p->buf.count;
	int row, i = 0;
	for (row = 0, i = first; i < last; i++, row++)
		terminal_scroll_print(row, p->buf.lines[i].text);

	bool at_top = (p->top == 0);
	bool at_bottom = (p->top + p->rows >= (int)p->buf.count);
	bool one_page = ((int)p->buf.count <= p->rows);

	char status[128] = "[q] quit";

	if (!one_page)
	{
		if (!at_top)
			strcat(status, "   [↑] prev page");
		if (!at_bottom)
			strcat(status, "   [↓] next page");
	}

	terminal_scroll_print(terminal_scroll_rows(), status);
	terminal_prompt_draw();
}

/* ============================================================
 *  INPUT
 * ============================================================ */

void screen_pager_handle_key(pager *p, int key)
{
	LOG("screen_pager_handle_key: key=%d top(before)=%d rows=%d buf.count=%zu",
		key, p->top, p->rows, p->buf.count);

	switch (key)
	{
	case 'q':
	case 'Q':
		p->screen.quit = true;
		LOG("screen_pager_handle_key: quit=true");
		break;

	case ' ':
	case DOWN:
		LOG("screen_pager_handle_key: DOWN/space pressed");
		p->top += p->rows;
		LOG("screen_pager_handle_key: top(after)=%d", p->top);

		if (p->top >= (int)p->buf.count && p->done)
		{
			p->screen.quit = true;
			LOG("screen_pager_handle_key: reached end, quit=true");
		}
		break;

	case '\n':
		LOG("screen_pager_handle_key: newline pressed");
		p->top++;
		LOG("screen_pager_handle_key: top(after)=%d", p->top);

		if (p->top >= (int)p->buf.count && p->done)
		{
			p->screen.quit = true;
			LOG("screen_pager_handle_key: reached end, quit=true");
		}
		break;

	case UP:
		LOG("screen_pager_handle_key: UP pressed");
		p->top -= p->rows;
		if (p->top < 0)
			p->top = 0;
		LOG("screen_pager_handle_key: top(after)=%d", p->top);
		break;

	default:
		LOG("screen_pager_handle_key: unhandled key=%d", key);
		break;
	}
}

void screen_pager_free(pager *p)
{
	for (size_t i = 0; i < p->buf.count; i++)
		free(p->buf.lines[i].text);
	free(p->buf.lines);
	free(p->cur);
}

/* ============================================================
 *  RUNNER
 * ============================================================ */

void screen_pager_columns(char data[][PATH_MAX], int count)
{
	LOG("screen_pager_columns: count=%d", count);

	pager p;
	screen_pager_start(&p);

	pager_column pc;
	_screen_pager_columns_init(&pc, data, count);

	_screen_pager_columns_build(&pc, data, count, &p);

	screen_pager_finish(&p);
	screen_pager_run(&p);
}