#ifndef LOG_H
#define LOG_H

#include "screen.h"

#define LOG_PAGE_SIZE 10

typedef struct
{
    screen screen;

    const char *title;
    char **lines;
    int count;

    int selected;
    int page;

    int result;

} log_screen;

void screen_log_init(log_screen *l, const char *title, char **lines, int count);
void screen_log_render(log_screen *l);
void screen_log_handle_key(log_screen *l, int key);
void screen_log_free(log_screen *l);

#endif