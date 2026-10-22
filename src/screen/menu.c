#include "main.h"
#include "menu.h"
#include "terminal.h"
#include "input.h"

/* ============================================================
 *  SETUP
 * ============================================================ */

void screen_menu_init(menu *m, menu_item *items, int count)
{
	memset(m, 0, sizeof(*m));

	m->items = items;
	m->count = count;
	m->selected = 0;
	m->page = 0;

	m->screen.type = SCREEN_MENU;
	m->screen.render = (void (*)(screen *))screen_menu_render;
	m->screen.handle_key = (void (*)(screen *, int))screen_menu_handle_key;
	m->screen.destroy = (void (*)(screen *))screen_menu_free;
}

/* ============================================================
 *  PROCESSING
 * ============================================================ */

void screen_menu_render(menu *m)
{
	int row = 0;

	int start = m->page * MENU_PAGE_SIZE;
	int end = start + MENU_PAGE_SIZE;
	if (end > m->count)
		end = m->count;

	for (int i = start; i < end; i++)
	{
		char line[256];
		int slot = (i - start) + 1;

		snprintf(line, sizeof(line), "[%d] %s", slot, m->items[i].label);

		if (i == m->selected)
			terminal_scroll_print_highlighted(row++, line);
		else
			terminal_scroll_print(row++, line);
	}

	int total_pages = (m->count + MENU_PAGE_SIZE - 1) / MENU_PAGE_SIZE;
	if (total_pages > 1)
	{
		char page_info[64];
		snprintf(page_info, sizeof(page_info), "-- Page %d/%d ([←][→] page) --", m->page + 1, total_pages);
		terminal_scroll_print(row++, page_info);
	}

	terminal_scroll_print(terminal_scroll_rows(), "[1-9] select  [↑][↓] move  [←][→] page  [q] quit");

	terminal_prompt_draw();
}

/* ============================================================
 *  INPUT
 * ============================================================ */

void screen_menu_handle_key(menu *m, int key)
{
	int total_pages = (m->count + MENU_PAGE_SIZE - 1) / MENU_PAGE_SIZE;

	if (key >= '1' && key <= '9')
	{
		int slot = key - '1';
		int target_index = (m->page * MENU_PAGE_SIZE) + slot;

		if (target_index < m->count)
		{
			m->selected = target_index;
		}
		return;
	}

	switch (key)
	{
	case UP:

		if (m->selected > 0)
		{
			m->selected--;
			m->page = m->selected / MENU_PAGE_SIZE;
		}

		break;

	case DOWN:

		if (m->selected < (m->count - 1))
		{
			m->selected++;
			m->page = m->selected / MENU_PAGE_SIZE;
		}

		break;

	case RIGHT:
		if (m->page < total_pages - 1)
		{
			m->page++;
			m->selected = m->page * MENU_PAGE_SIZE;
		}
		break;

	case LEFT:
		if (m->page > 0)
		{
			m->page--;
			m->selected = m->page * MENU_PAGE_SIZE;
		}
		break;

	case 'q':
	case 'Q':

		m->screen.quit = true;
		break;

	case '\n':
	case ' ':

		m->result = m->selected;
		m->accepted = true;
		m->screen.quit = true;

		break;
	}
}

void screen_menu_free(menu *m)
{
	(void)m;
}

int test_menu(void)
{
	menu_item items[] = {
		{
			.label = "Security Logs",
		},
		{
			.label = "Personnel Files",
		},
		{
			.label = "Reactor Status",
		},
		{
			.label = "Engineering Controls",
		},
		{
			.label = "Maintenance Directives",
		},
		{
			.label = "Vault Archives",
		},
		{
			.label = "Terminal Settings",
		},
		{
			.label = "User Management",
		},
		{
			.label = "Overseer Directives",
		},
		{
			.label = "Sub-level Diagnostics",
		},
		{
			.label = "Exit",
		}};

	menu m;

	screen_menu_init(&m, items, 11);

	terminal_scroll_clear();

	while (!m.screen.quit)
	{
		int key;

		screen_menu_render(&m);

		if (!terminal_poll_key(&key))
			key = input_read_char();

		screen_menu_handle_key(&m, key);

		terminal_scroll_clear();
	}

	return m.accepted ? m.result : -1;
}