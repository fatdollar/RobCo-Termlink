#include "screen.h"
#include "terminal.h"
#include "input.h"

#define SCREEN_STACK_MAX 16

static struct
{
    screen *stack[SCREEN_STACK_MAX];
    int depth;
} g_screens;

void screen_push(screen *s)
{
    if (g_screens.depth >= SCREEN_STACK_MAX)
        return;

    g_screens.stack[g_screens.depth++] = s;
}

void screen_pop(void)
{
    if (!g_screens.depth)
        return;

    screen *s = g_screens.stack[--g_screens.depth];

    if (s->destroy)
        s->destroy(s);
}

screen *screen_current(void)
{
    if (!g_screens.depth)
        return NULL;

    return g_screens.stack[g_screens.depth - 1];
}

void screen_loop(void)
{
    while (screen_current())
    {
        screen *s = screen_current();

        terminal_scroll_clear();
        s->render(s);

        int key;

        if (!terminal_poll_key(&key))
            key = input_read_char();

        s->handle_key(s, key);

        if (s->quit)
            screen_pop();
    }

    terminal_scroll_clear();
    terminal_prompt_draw();
}