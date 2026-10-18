#ifndef TERMINAL_H
#define TERMINAL_H

#include "input_state.h"

#define PRINT_DELAY 7500
#define MAX_ARGS 64
#define MAX_COMPLETIONS 128
#define PROMPT_COL 2
#define MAX_SCROLL_LINES 256

extern struct termios terminal_orig_termios;

typedef struct
{
    int row;
    int start_col;
    int length;
} scroll_line;

extern scroll_line scroll_lines[MAX_SCROLL_LINES];
extern int scroll_line_count;

extern int terminal_rows;
extern int terminal_cols;

extern int terminal_scroll_top;
extern int terminal_scroll_bottom;
extern int terminal_cursor_line;

extern int termlink_job_control_enabled;

void terminal_sigint(int sig);

int terminal_scroll_rows(void);
int terminal_scroll_cols(void);
bool terminal_poll_key(int *key);

void terminal_restore(void);
void terminal_setup(void);
void terminal_update_size(void);

void terminal_slow_putc(char c);
void terminal_slow_print_at(int line, const char *s);
void terminal_slow_print(const char *s);
void terminal_write_scroll_line(int row, const char *text);
void terminal_write_status(const char *text);

void terminal_clear_scroll(void);
void terminal_draw_prompt(void);
void terminal_print_header(void);

void terminal_expand_tilde(const char **target);

void terminal_process(input_state *st, const char *cmd);

#endif