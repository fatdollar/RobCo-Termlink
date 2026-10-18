#include "main.h"
#include "pager.h"
#include "terminal.h"
#include "input.h"

static void pager_buf_push(struct pager_buffer *b, const char *s, size_t n)
{
    if (b->count == b->cap)
    {
        b->cap = b->cap ? b->cap * 2 : 64;
        b->lines = realloc(b->lines, b->cap * sizeof(struct pager_line));
    }
    b->lines[b->count].text = malloc(n + 1);
    memcpy(b->lines[b->count].text, s, n);
    b->lines[b->count].text[n] = 0;
    b->lines[b->count].len = n;
    b->count++;
}

void pager_start(struct pager *p)
{
    pager_init(p, terminal_scroll_rows(), terminal_cols);
}

void pager_print_line(struct pager *p, const char *s)
{
    pager_feed(p, s, strlen(s));
    pager_feed(p, "\n", 1);
}

void pager_finish(struct pager *p)
{
    p->done = true;
}

void pager_init(struct pager *p, int rows, int cols)
{
    memset(p, 0, sizeof(*p));
    p->rows = rows;
    p->cols = cols;
    p->cur_cap = 256;
    p->cur = malloc(p->cur_cap);
}

void pager_free(struct pager *p)
{
    for (size_t i = 0; i < p->buf.count; i++)
        free(p->buf.lines[i].text);
    free(p->buf.lines);
    free(p->cur);
}

void pager_feed(struct pager *p, const char *data, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        char c = data[i];
        if (c == '\n')
        {
            pager_buf_push(&p->buf, p->cur, p->cur_len);
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

void pager_get_input(struct pager *p)
{
    while (1)
    {
        int key;

        if (!terminal_poll_key(&key))
            key = input_read_char(); // BLOCK

        int old_top = p->top;
        pager_handle_key(p, key);

        if (p->quit)
        {
            terminal_clear_scroll();
            terminal_draw_prompt();
            break;
        }

        if (p->top != old_top)
        {
            terminal_clear_scroll();
            pager_render(p);
        }
    }
}

void pager_render(struct pager *p)
{
    int first = p->top;
    int last = first + p->rows;
    if (last > (int)p->buf.count)
        last = p->buf.count;

    for (int row = 0, i = first; i < last; i++, row++)
        terminal_write_scroll_line(row, p->buf.lines[i].text);

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

    terminal_write_status(status);
    terminal_draw_prompt();
}

void pager_handle_key(struct pager *p, int key)
{
    LOG("pager_handle_key: key=%d top(before)=%d rows=%d buf.count=%zu",
        key, p->top, p->rows, p->buf.count);

    switch (key)
    {
    case 'q':
    case 'Q':
        p->quit = true;
        LOG("pager_handle_key: quit=true");
        break;

    case ' ':
    case DOWN:
        LOG("pager_handle_key: DOWN/space pressed");
        p->top += p->rows;
        LOG("pager_handle_key: top(after)=%d", p->top);

        if (p->top >= (int)p->buf.count && p->done)
        {
            p->quit = true;
            LOG("pager_handle_key: reached end, quit=true");
        }
        break;

    case '\n':
        LOG("pager_handle_key: newline pressed");
        p->top++;
        LOG("pager_handle_key: top(after)=%d", p->top);

        if (p->top >= (int)p->buf.count && p->done)
        {
            p->quit = true;
            LOG("pager_handle_key: reached end, quit=true");
        }
        break;

    case UP:
        LOG("pager_handle_key: UP pressed");
        p->top -= p->rows;
        if (p->top < 0)
            p->top = 0;
        LOG("pager_handle_key: top(after)=%d", p->top);
        break;

    default:
        LOG("pager_handle_key: unhandled key=%d", key);
        break;
    }
}

void pager_run(struct pager *p)
{
    // Clear scroll region
    terminal_clear_scroll();

    // Render first page
    LOG("pager rows=%d buf.count=%zu top=%d", p->rows, p->buf.count, p->top);
    pager_render(p);

    pager_get_input(p);
}