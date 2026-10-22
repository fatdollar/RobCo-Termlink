#include "main.h"
#include "diagnostic.h"
#include "terminal.h"
#include "input.h"

/* ============================================================
 *  SETUP
 * ============================================================ */

void screen_diagnostic_init(diagnostic *d, const char *title, diagnostic_metric *metrics, int count)
{
    memset(d, 0, sizeof(*d));

    d->title = title;
    d->metrics = metrics;
    d->count = count > DIAG_MAX_METRICS ? DIAG_MAX_METRICS : count;
    d->result = 0;

    d->screen.type = SCREEN_DIAGNOSTIC;

    d->screen.render = (void (*)(screen *))screen_diagnostic_render;
    d->screen.handle_key = (void (*)(screen *, int))screen_diagnostic_handle_key;
    d->screen.destroy = (void (*)(screen *))screen_diagnostic_free;
}

/* ============================================================
 *  PROCESSING
 * ============================================================ */

void screen_diagnostic_render(diagnostic *d)
{
    int row = 0;

    char title_buf[128];
    snprintf(title_buf, sizeof(title_buf), "=================================================");
    terminal_scroll_print(row++, title_buf);

    snprintf(title_buf, sizeof(title_buf), "  DIAGNOSTICS: %s", d->title);
    terminal_scroll_print(row++, title_buf);

    snprintf(title_buf, sizeof(title_buf), "=================================================");
    terminal_scroll_print(row++, title_buf);
    terminal_scroll_print(row++, "");

    for (int i = 0; i < d->count; i++)
    {
        int pct = d->metrics[i].percentage;
        if (pct < 0)
            pct = 0;
        if (pct > 100)
            pct = 100;

        int blocks = pct / 10;
        char bar_buf[16] = "";
        for (int b = 0; b < 10; b++)
        {
            strcat(bar_buf, (b < blocks) ? "#" : ".");
        }

        char line[256];
        const char *lbl = d->metrics[i].label ? d->metrics[i].label : "SYSTEM";
        snprintf(line, sizeof(line), "  %-12s [%s] %3d%%", lbl, bar_buf, pct);
        terminal_scroll_print(row++, line);
    }

    terminal_scroll_print(row++, "");
    terminal_scroll_print(row++, "-------------------------------------------------");
    terminal_scroll_print(terminal_scroll_rows(), "[ENTER / Q] RETURN TO TERMINAL");
    terminal_prompt_draw();
}

/* ============================================================
 *  INPUT
 * ============================================================ */

void screen_diagnostic_handle_key(diagnostic *d, int key)
{
    switch (key)
    {
    case '\n':
    case '\r':
    case ' ':
    case 'q':
    case 'Q':
        d->screen.quit = true;
        break;
    }
}

void screen_diagnostic_free(diagnostic *d)
{
    (void)d;
}