#include "main.h"
#include "alert.h"
#include "terminal.h"
#include "input.h"

/* ============================================================
 *  SETUP
 * ============================================================ */

void screen_alert_init(alert *a, const char *title, const char *message)
{
    memset(a, 0, sizeof(*a));

    a->title = title;
    a->message = message;
    a->result = 0;

    a->screen.type = SCREEN_ALERT;
    a->screen.render = (void (*)(screen *))screen_alert_render;
    a->screen.handle_key = (void (*)(screen *, int))screen_alert_handle_key;
    a->screen.destroy = (void (*)(screen *))screen_alert_free;
}

/* ============================================================
 *  PROCESSING
 * ============================================================ */

void screen_alert_render(alert *s)
{
    alert *a = (alert *)s;
    int row = 2;

    terminal_scroll_print(row++, "=================================================");

    char title_buf[128];
    snprintf(title_buf, sizeof(title_buf), "  WARNING: %s", a->title);
    terminal_scroll_print(row++, title_buf);

    terminal_scroll_print(row++, "=================================================");
    terminal_scroll_print(row++, "");

    char msg_buf[256];
    snprintf(msg_buf, sizeof(msg_buf), "  %s", a->message);
    terminal_scroll_print(row++, msg_buf);

    terminal_scroll_print(row++, "");
    terminal_scroll_print(row++, "-------------------------------------------------");
    terminal_scroll_print(row++, "  [ENTER / SPACE] ACKNOWLEDGE");
    terminal_scroll_print(row++, "=================================================");

    terminal_prompt_draw();
}

/* ============================================================
 *  INPUT
 * ============================================================ */

void screen_alert_handle_key(alert *s, int key)
{
    alert *a = (alert *)s;

    switch (key)
    {
    case '\n':
    case ' ':
    case '\r':
        a->result = 1;
        a->screen.quit = true;
        break;

    case 'q':
    case 'Q':
        a->result = 0;
        a->screen.quit = true;
        break;
    }
}

void screen_alert_free(alert *a)
{
    (void)a;
}