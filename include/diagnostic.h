#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H

#include "screen.h"

#define DIAG_MAX_METRICS 8

typedef struct
{
    const char *label;
    int percentage; /* 0 to 100 */
} diagnostic_metric;

typedef struct
{
    screen screen;

    const char *title;
    diagnostic_metric *metrics;
    int count;

    int result;

} diagnostic;

void screen_diagnostic_init(diagnostic *d, const char *title, diagnostic_metric *metrics, int count);
void screen_diagnostic_render(diagnostic *d);
void screen_diagnostic_handle_key(diagnostic *d, int key);
void screen_diagnostic_free(diagnostic *d);

#endif