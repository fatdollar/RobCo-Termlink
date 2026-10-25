#ifndef THEME_H
#define THEME_H

#include <stdbool.h>

typedef struct
{
    int fg_r, fg_g, fg_b;
    int bg_r, bg_g, bg_b;
    char cursor_hex[16];
    char preset[32];
} termlink_theme;

void theme_apply(void);
void theme_load_config(void);
void theme_save_config(const char *preset_name);

extern termlink_theme current_theme;

#endif