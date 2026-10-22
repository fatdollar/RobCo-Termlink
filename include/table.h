#ifndef TABLE_H
#define TABLE_H

#include "screen.h"

#define TABLE_MAX_COLS 4
#define TABLE_PAGE_SIZE 8

typedef struct
{
    const char *label;
    int width;
} table_column;

typedef struct
{
    const char *cells[TABLE_MAX_COLS];
} table_row;

typedef struct
{
    screen screen;

    const char *title;
    table_column *columns;
    int col_count;

    table_row *rows;
    int row_count;

    int selected;
    int page;

    int result;
    bool accepted;

} table;

void screen_table_init(table *t, const char *title, table_column *cols, int col_count, table_row *rows, int row_count);
void screen_table_render(table *t);
void screen_table_handle_key(table *t, int key);
void screen_table_free(table *t);

#endif