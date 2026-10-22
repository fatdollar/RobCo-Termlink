#ifndef SCREEN_H
#define SCREEN_H

#include "main.h"

typedef struct screen screen;

typedef enum
{
    SCREEN_CONSOLE,
    SCREEN_PAGER,
    SCREEN_MENU,
    SCREEN_TABLE,
    SCREEN_STATUS,
    SCREEN_LOG,
    SCREEN_ALERT,
    SCREEN_DIAGNOSTIC
} screen_type;

struct screen
{
    screen_type type;
    
    void (*render)(screen *s);
    void (*handle_key)(screen *s, int key);
    void (*destroy)(screen *s);

    bool quit;
};

void screen_push(screen *s);
void screen_pop(void);
screen *screen_current(void);

void screen_loop(void);

#endif