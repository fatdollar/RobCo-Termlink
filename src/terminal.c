#include "main.h"
#include "terminal.h"
#include "theme.h"
#include "debug.h"

struct termios terminal_orig_termios;

int terminal_rows = 24;
int terminal_cols = 80;

int terminal_scroll_top = 5;
int terminal_scroll_bottom = 0;
int terminal_cursor_line = 5;

void terminal_restore(void)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &terminal_orig_termios);
}

void terminal_setup(void)
{
	struct termios t;
	LOG("terminal_setup: entering raw mode");
	tcgetattr(STDIN_FILENO, &terminal_orig_termios);
	atexit(terminal_restore);

	t = terminal_orig_termios;
	t.c_lflag &= ~(ICANON | ECHO);
	t.c_cc[VMIN] = 1;
	t.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &t);

	theme_apply_default();
	LOG("terminal_setup: rows=%d cols=%d", terminal_rows, terminal_cols);
}

void terminal_update_size(void)
{
	LOG("terminal_update_size: before ioctl rows=%d cols=%d", terminal_rows, terminal_cols);

	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0)
	{
		terminal_rows = ws.ws_row;
		terminal_cols = ws.ws_col;
		LOG("terminal_update_size: ioctl rows=%d cols=%d", terminal_rows, terminal_cols);
	}

	terminal_scroll_bottom = terminal_rows - 1;

	LOG("terminal_update_size: scroll_top=%d scroll_bottom=%d cursor_line=%d",
		terminal_scroll_top, terminal_scroll_bottom, terminal_cursor_line);

	if (terminal_cursor_line < terminal_scroll_top || terminal_cursor_line >= terminal_scroll_bottom)
	{
		LOG("terminal_update_size: cursor_line reset to scroll_top");
		terminal_cursor_line = terminal_scroll_top;
	}
}

void terminal_slow_putc(char c)
{
	putchar(c);
	fflush(stdout);
	usleep(PRINT_DELAY);
}

void terminal_slow_print_at(int line, const char *s)
{
	LOG("terminal_slow_print_at: line=%d text='%s'", line, s);
	printf("\033[%d;1H", line);
	fflush(stdout);

	for (const char *p = s; *p; ++p)
		terminal_slow_putc(*p);
}

void terminal_slow_print(const char *s)
{
	terminal_slow_print_at(terminal_cursor_line, s);
	terminal_cursor_line++;
}

void terminal_clear_scroll(void)
{
	int last_scroll_line = terminal_rows - 1; // bottom row is prompt

	LOG("terminal_clear_scroll: clearing lines %d to %d",
		terminal_scroll_top, last_scroll_line - 1);

	for (int i = terminal_scroll_top; i < last_scroll_line; i++)
	{
		printf("\033[%d;1H", i);
		printf("\033[2K"); // clear entire line
	}

	terminal_cursor_line = terminal_scroll_top;
	LOG("terminal_clear_scroll: cursor_line=%d", terminal_cursor_line);
}

void terminal_draw_prompt(void)
{
	LOG("terminal_draw_prompt: moving cursor to bottom row %d", terminal_rows);
	printf("\033[%d;1H", terminal_rows);

	terminal_cursor_line = terminal_scroll_top;

	LOG("terminal_draw_prompt: printing prompt '>'");
	putchar('>');
}

void terminal_print_header(void)
{
	LOG("terminal_print_header: clearing screen");
	printf("\033[2J\033[H");

	LOG("terminal_print_header: printing header");
	terminal_slow_print_at(1, "=============================================");
	terminal_slow_print_at(2, "      RobCo Industries (TM) Termlink v1.0");
	terminal_slow_print_at(3, "=============================================");
	terminal_slow_print_at(4, "");

	LOG("terminal_print_header: clearing scroll region");
	terminal_clear_scroll();
}