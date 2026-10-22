#ifndef TERMLINK_TABLE_H
#define TERMLINK_TABLE_H

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

} table_screen;

void screen_table_init(table_screen *t, const char *title, table_column *cols, int col_count, table_row *rows, int row_count);
void screen_table_render(table_screen *t);
void screen_table_handle_key(table_screen *t, int key);
void screen_table_free(table_screen *t);

#endif