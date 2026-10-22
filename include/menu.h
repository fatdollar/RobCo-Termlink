#ifndef MENU_H
#define MENU_H

#include "screen.h"

#define MENU_PAGE_SIZE 9

typedef void (*menu_action)(void *);

typedef struct
{
	const char *label;
} menu_item;

typedef struct
{
	screen screen;

	menu_item *items;

	int count;

	int selected;

	int page;

	int result;
	bool accepted;

} menu;

void screen_menu_init(menu *m, menu_item *items, int count);
void screen_menu_render(menu *m);
void screen_menu_handle_key(menu *m, int key);
void screen_menu_free(menu *m);

#endif