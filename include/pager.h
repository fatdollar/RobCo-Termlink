#ifndef TERMLINK_PAGER_H
#define TERMLINK_PAGER_H

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
    pager_buffer buf;
    char *cur;
    size_t cur_len;
    size_t cur_cap;

    int top;
    int rows;
    int cols;

    bool done;
    bool quit;
} pager;

void pager_start(pager *p);
void pager_print_line(pager *p, const char *s);
void pager_finish(pager *p);
void pager_init(pager *p, int rows, int cols);
void pager_free(pager *p);
void pager_feed(pager *p, const char *data, size_t n);
void pager_columns(char matches[][PATH_MAX], int count);
void pager_render(pager *p);
void pager_handle_key(pager *p, int key);
void pager_run(pager *p);

#endif