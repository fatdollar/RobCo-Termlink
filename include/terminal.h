#ifndef TERMINAL_H
#define TERMINAL_H

extern struct termios terminal_orig_termios;

extern int terminal_rows;
extern int terminal_cols;

extern int terminal_scroll_top;
extern int terminal_scroll_bottom;
extern int terminal_cursor_line;

#define PRINT_DELAY 7500
#define MAX_ARGS 64
#define MAX_COMPLETIONS 128
#define PROMPT_COL 2

void terminal_restore(void);
void terminal_setup(void);
void terminal_update_size(void);

void terminal_slow_putc(char c);
void terminal_slow_print_at(int line, const char *s);
void terminal_slow_print(const char *s);

void terminal_clear_scroll(void);
void terminal_draw_prompt(void);
void terminal_print_header(void);

#endif