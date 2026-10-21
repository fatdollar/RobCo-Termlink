#ifndef TERMLINK_PAGER_H
#define TERMLINK_PAGER_H

#include "screen.h"

typedef struct
{
	char *text;
	size_t len;
} pager_line;

typedef struct
{
	pager_line *lines;
	size_t count;
	size_t cap;
} pager_buffer;

typedef struct
{
	int term_width;
	int col_width;
	int cols;
} pager_column;

typedef struct
{
	screen screen;

	pager_buffer buf;
	char *cur;
	size_t cur_len;
	size_t cur_cap;

	int top;
	int rows;
	int cols;

	bool done;
} pager;

void screen_pager_init(pager *p, int rows, int cols);
void screen_pager_start(pager *p);
void pager_print_line(pager *p, const char *s);
void screen_pager_finish(pager *p);
void screen_pager_feed(pager *p, const char *data, size_t n);
void screen_pager_run(pager *p);
void screen_pager_render(pager *p);
void screen_pager_handle_key(pager *p, int key);
void screen_pager_free(pager *p);
void screen_pager_columns(char matches[][PATH_MAX], int count);

#endif