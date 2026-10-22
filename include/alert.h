#ifndef ALERT_H
#define ALERT_H

#include "screen.h"

typedef struct
{
    screen screen;
    const char *title;
    const char *message;
    int result;
} alert;

void screen_alert_init(alert *a, const char *title, const char *message);
void screen_alert_render(alert *s);
void screen_alert_handle_key(alert *s, int key);
void screen_alert_free(alert *s);

#endif