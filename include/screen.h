#ifndef SCREEN_H
#define SCREEN_H

#include "main.h"

typedef struct screen screen;

struct screen
{
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