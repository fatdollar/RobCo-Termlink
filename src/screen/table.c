#include "main.h"
#include "table.h"
#include "terminal.h"
#include "input.h"

/* ============================================================
 *  SETUP
 * ============================================================ */

void screen_table_init(table *t, const char *title, table_column *cols, int col_count, table_row *rows, int row_count)
{
    memset(t, 0, sizeof(*t));

    t->title = title;
    t->columns = cols;
    t->col_count = col_count > TABLE_MAX_COLS ? TABLE_MAX_COLS : col_count;
    t->rows = rows;
    t->row_count = row_count;
    t->selected = 0;
    t->page = 0;
    t->result = -1;
    t->accepted = false;

    t->screen.type = SCREEN_TABLE;

    t->screen.render = (void (*)(screen *))screen_table_render;
    t->screen.handle_key = (void (*)(screen *, int))screen_table_handle_key;
    t->screen.destroy = (void (*)(screen *))screen_table_free;
}

/* ============================================================
 *  PROCESSING
 * ============================================================ */

void screen_table_render(table *t)
{
    int row = 0;

    /* Title */
    char title_line[128];
    snprintf(title_line, sizeof(title_line), "=== %s ===", t->title);
    terminal_scroll_print(row++, title_line);
    terminal_scroll_print(row++, "");

    /* Header Row */
    char header_buf[256] = " ";
    for (int c = 0; c < t->col_count; c++)
    {
        char col_fmt[32];
        snprintf(col_fmt, sizeof(col_fmt), "%%-%ds ", t->columns[c].width);
        char formatted_col[64];
        snprintf(formatted_col, sizeof(formatted_col), col_fmt, t->columns[c].label);
        strncat(header_buf, formatted_col, sizeof(header_buf) - strlen(header_buf) - 1);
    }
    terminal_scroll_print(row++, header_buf);
    terminal_scroll_print(row++, "-------------------------------------------------");

    /* Data Rows (Paged) */
    int start = t->page * TABLE_PAGE_SIZE;
    int end = start + TABLE_PAGE_SIZE;
    if (end > t->row_count)
        end = t->row_count;

    for (int i = start; i < end; i++)
    {
        char row_buf[256];
        if (i == t->selected)
        {
            snprintf(row_buf, sizeof(row_buf), ">");
        }
        else
        {
            snprintf(row_buf, sizeof(row_buf), " ");
        }

        for (int c = 0; c < t->col_count; c++)
        {
            char col_fmt[32];
            snprintf(col_fmt, sizeof(col_fmt), " %%-%ds", t->columns[c].width);
            char cell_formatted[64];
            const char *val = t->rows[i].cells[c] ? t->rows[i].cells[c] : "";
            snprintf(cell_formatted, sizeof(cell_formatted), col_fmt, val);
            strncat(row_buf, cell_formatted, sizeof(row_buf) - strlen(row_buf) - 1);
        }

        terminal_scroll_print(row++, row_buf);
    }

    int total_pages = (t->row_count + TABLE_PAGE_SIZE - 1) / TABLE_PAGE_SIZE;
    if (total_pages > 1)
    {
        char page_info[64];
        snprintf(page_info, sizeof(page_info), "-- Page %d/%d ([←][→] page) --", t->page + 1, total_pages);
        terminal_scroll_print(row++, page_info);
    }

    terminal_scroll_print(terminal_scroll_rows(), "[↑][↓] move  [←][→] page  [ENTER] select  [q] quit");
    terminal_prompt_draw();
}

/* ============================================================
 *  INPUT
 * ============================================================ */

void screen_table_handle_key(table *t, int key)
{
    int total_pages = (t->row_count + TABLE_PAGE_SIZE - 1) / TABLE_PAGE_SIZE;

    switch (key)
    {
    case UP:

        if (t->selected > 0)
        {
            t->selected--;
            t->page = t->selected / TABLE_PAGE_SIZE;
        }

        break;

    case DOWN:

        if (t->selected < (t->row_count - 1))
        {
            t->selected++;
            t->page = t->selected / TABLE_PAGE_SIZE;
        }

        break;

    case RIGHT:
        if (t->page < total_pages - 1)
        {
            t->page++;
            t->selected = t->page * TABLE_PAGE_SIZE;
        }
        break;

    case LEFT:
        if (t->page > 0)
        {
            t->page--;
            t->selected = t->page * TABLE_PAGE_SIZE;
        }
        break;

    case 'q':
    case 'Q':

        t->screen.quit = true;
        break;

    case '\n':
    case ' ':

        t->result = t->selected;
        t->accepted = true;
        t->screen.quit = true;

        break;
    }
}

void screen_table_free(table *t)
{
    (void)t;
}