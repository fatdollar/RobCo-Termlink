#ifndef TERMLINK_PAGER_H
#define TERMLINK_PAGER_H

#include <stddef.h>
#include <stdbool.h>

struct pager_line
{
    char *text;
    size_t len;
};

struct pager_buffer
{
    struct pager_line *lines;
    size_t count;
    size_t cap;
};

struct pager
{
    struct pager_buffer buf;
    char *cur;
    size_t cur_len;
    size_t cur_cap;

    int top;
    int rows;
    int cols;

    bool done;
    bool quit;
};

void pager_init(struct pager *p, int rows, int cols);
void pager_free(struct pager *p);
void pager_feed(struct pager *p, const char *data, size_t n);
void pager_render(struct pager *p);
void pager_handle_key(struct pager *p, int key);

#endif