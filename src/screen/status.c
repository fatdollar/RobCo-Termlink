#include "main.h"
#include "status.h"
#include "terminal.h"
#include "input.h"

/* ============================================================
 *  SETUP
 * ============================================================ */

void screen_status_init(status *st, const char *title, status_metric *metrics, int count)
{
    memset(st, 0, sizeof(*st));

    st->title = title;
    st->metrics = metrics;
    st->count = count > STATUS_MAX_METRICS ? STATUS_MAX_METRICS : count;
    st->result = 0;

    st->screen.type = SCREEN_STATUS;

    st->screen.render = (void (*)(screen *))screen_status_render;
    st->screen.handle_key = (void (*)(screen *, int))screen_status_handle_key;
    st->screen.destroy = (void (*)(screen *))screen_status_free;
}

/* ============================================================
 *  PROCESSING
 * ============================================================ */

void screen_status_render(status *st)
{
    int row = 0;

    /* Dashboard Title Block */
    char title_buf[128];
    snprintf(title_buf, sizeof(title_buf), "=================================================");
    terminal_scroll_print(row++, title_buf);

    snprintf(title_buf, sizeof(title_buf), "  SYSTEM STATUS: %s", st->title);
    terminal_scroll_print(row++, title_buf);

    snprintf(title_buf, sizeof(title_buf), "=================================================");
    terminal_scroll_print(row++, title_buf);
    terminal_scroll_print(row++, "");

    /* Metric Key-Value Display Grid */
    for (int i = 0; i < st->count; i++)
    {
        char metric_line[256];
        const char *val = st->metrics[i].value ? st->metrics[i].value : "UNKNOWN";
        const char *lbl = st->metrics[i].label ? st->metrics[i].label : "METRIC";

        if (st->metrics[i].is_warning)
        {
            snprintf(metric_line, sizeof(metric_line), "  %-24s [!] %s", lbl, val);
        }
        else
        {
            snprintf(metric_line, sizeof(metric_line), "  %-24s     %s", lbl, val);
        }

        terminal_scroll_print(row++, metric_line);
    }

    terminal_scroll_print(row++, "");
    terminal_scroll_print(row++, "-------------------------------------------------");

    /* Footer */
    terminal_scroll_print(terminal_scroll_rows(), "[ENTER / Q] RETURN TO TERMINAL");
    terminal_prompt_draw();
}

/* ============================================================
 *  INPUT
 * ============================================================ */

void screen_status_handle_key(status *st, int key)
{
    switch (key)
    {
    case '\n':
    case '\r':
    case ' ':
    case 'q':
    case 'Q':
        st->screen.quit = true;
        break;
    }
}

void screen_status_free(status *st)
{
    (void)st;
}