#include "main.h"
#include "log.h"
#include "terminal.h"
#include "input.h"

/* ============================================================
 *  SETUP
 * ============================================================ */

void screen_log_init(log *l, const char *title, char **lines, int count)
{
    memset(l, 0, sizeof(*l));

    l->title = title;
    l->lines = lines;
    l->count = count;
    l->selected = 0;
    l->page = 0;
    l->result = 0;

    l->screen.type = SCREEN_LOG;

    l->screen.render = (void (*)(screen *))screen_log_render;
    l->screen.handle_key = (void (*)(screen *, int))screen_log_handle_key;
    l->screen.destroy = (void (*)(screen *))screen_log_free;
}

/* ============================================================
 *  PROCESSING
 * ============================================================ */

void screen_log_render(log *l)
{
    int row = 0;

    /* Title Block */
    char title_buf[128];
    snprintf(title_buf, sizeof(title_buf), "=================================================");
    terminal_scroll_print(row++, title_buf);

    snprintf(title_buf, sizeof(title_buf), "  LOG STREAM: %s", l->title);
    terminal_scroll_print(row++, title_buf);

    snprintf(title_buf, sizeof(title_buf), "=================================================");
    terminal_scroll_print(row++, title_buf);
    terminal_scroll_print(row++, "");

    /* Paged Log Entries */
    int start = l->page * LOG_PAGE_SIZE;
    int end = start + LOG_PAGE_SIZE;
    if (end > l->count)
        end = l->count;

    for (int i = start; i < end; i++)
    {
        char line_buf[256];
        const char *content = l->lines[i] ? l->lines[i] : "";

        if (i == l->selected)
        {
            snprintf(line_buf, sizeof(line_buf), "> %s", content);
        }
        else
        {
            snprintf(line_buf, sizeof(line_buf), "  %s", content);
        }

        terminal_scroll_print(row++, line_buf);
    }

    /* Page Footer / Navigation indicator */
    int total_pages = (l->count + LOG_PAGE_SIZE - 1) / LOG_PAGE_SIZE;
    if (total_pages > 1)
    {
        char page_info[64];
        snprintf(page_info, sizeof(page_info), "-- Page %d/%d ([←][→] page) --", l->page + 1, total_pages);
        terminal_scroll_print(row++, page_info);
    }

    terminal_scroll_print(terminal_scroll_rows(), "[↑][↓] browse  [←][→] page  [f] filter  [s] search  [q] quit");
    terminal_prompt_draw();
}

/* ============================================================
 *  INPUT
 * ============================================================ */

void screen_log_handle_key(log *l, int key)
{
    int total_pages = (l->count + LOG_PAGE_SIZE - 1) / LOG_PAGE_SIZE;

    switch (key)
    {
    case UP:

        if (l->selected > 0)
        {
            l->selected--;
            l->page = l->selected / LOG_PAGE_SIZE;
        }

        break;

    case DOWN:

        if (l->selected < (l->count - 1))
        {
            l->selected++;
            l->page = l->selected / LOG_PAGE_SIZE;
        }

        break;

    case RIGHT:
        if (l->page < total_pages - 1)
        {
            l->page++;
            l->selected = l->page * LOG_PAGE_SIZE;
        }
        break;

    case LEFT:
        if (l->page > 0)
        {
            l->page--;
            l->selected = l->page * LOG_PAGE_SIZE;
        }
        break;

    case 'q':
    case 'Q':
        l->screen.quit = true;
        break;

    case 'f':
    case 'F':
        /* Stub for filter handling */
        break;

    case 's':
    case 'S':
        /* Stub for search handling */
        break;
    }
}

void screen_log_free(log *l)
{
    (void)l;
}