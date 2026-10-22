#ifndef STATUS_H
#define STATUS_H

#include "screen.h"

#define STATUS_MAX_METRICS 12

typedef struct
{
    const char *label;
    const char *value;
    bool is_warning;
} status_metric;

typedef struct
{
    screen screen;

    const char *title;
    status_metric *metrics;
    int count;

    int result;

} status;

void screen_status_init(status *st, const char *title, status_metric *metrics, int count);
void screen_status_render(status *st);
void screen_status_handle_key(status *st, int key);
void screen_status_free(status *st);

#endif