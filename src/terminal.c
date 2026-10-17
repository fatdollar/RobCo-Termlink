#include "main.h"
#include "terminal.h"
#include "builtin.h"
#include "external.h"
#include "theme.h"
#include "input.h"
#include "cursor.h"

struct termios terminal_orig_termios;

scroll_line scroll_lines[MAX_SCROLL_LINES];
int scroll_line_count;

int terminal_rows = 24;
int terminal_cols = 80;

int terminal_scroll_top = 5;
int terminal_scroll_bottom = 0;
int terminal_cursor_line = 5;

int termlink_job_control_enabled = 0;

void terminal_sigint(int sig)
{
	LOG("terminal_sigint: sigint skipped for termlink");
}

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
	t.c_lflag |= ISIG;
	t.c_cc[VMIN] = 1;
	t.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &t);

	if (termlink_job_control_enabled)
	{
		pid_t shell_pgid = getpid();
		setpgid(shell_pgid, shell_pgid);
		tcsetpgrp(STDIN_FILENO, shell_pgid);

		struct sigaction sa;
		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = SIG_IGN;
		sigaction(SIGINT, &sa, NULL);

		LOG("terminal_setup: job control active");
	}
	else
	{
		LOG("terminal_setup: fallback mode (no job control)");
	}

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

void terminal_slow_clear_putc()
{
	putchar(' ');
	fflush(stdout);
	usleep(PRINT_DELAY / 2);
}

void terminal_slow_print_at(int line, const char *s)
{
	LOG("terminal_slow_print_at: line=%d text='%s'", line, s);
	printf("\033[%d;1H", line);
	fflush(stdout);

	for (const char *p = s; *p; ++p)
		terminal_slow_putc(*p);

	// Record printed content length
	if (scroll_line_count < MAX_SCROLL_LINES)
		scroll_lines[scroll_line_count++] = (scroll_line){line, (int)strlen(s)};
}

void terminal_slow_print(const char *s)
{
	terminal_slow_print_at(terminal_cursor_line, s);
	terminal_cursor_line++;
}

void terminal_clear_scroll(void)
{
	int prompt_row = terminal_rows; // bottom row

	LOG("terminal_clear_scroll: slow-clearing %d lines", scroll_line_count);

	// Slow-clear only lines that contain content
	for (int i = 0; i < scroll_line_count; i++)
	{
		int row = scroll_lines[i].row;
		int len = scroll_lines[i].length;

		if (row >= terminal_scroll_top && row < prompt_row)
		{
			printf("\033[%d;1H", row);

			for (int c = 0; c < len; c++)
				terminal_slow_clear_putc(' ');
		}
	}

	// Fast-clear the rest of the scroll region
	for (int row = terminal_scroll_top; row < prompt_row - 1; row++)
	{
		printf("\033[%d;1H", row);
		printf("\033[2K");
	}

	// Reset scroll-line tracker
	scroll_line_count = 0;

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

void terminal_expand_tilde(const char **target)
{
	LOG("expand_tilde: input='%s'", *target);

	if ((*target)[0] == '~')
	{
		LOG("expand_tilde: tilde detected");

		const char *home = getenv("HOME");
		LOG("expand_tilde: HOME='%s'", home ? home : "(null)");

		if (!home)
			home = "/";

		static char expanded[PATH_MAX];

		if ((*target)[1] == '/' || (*target)[1] == '\0')
		{
			// "~" or "~/something"
			LOG("expand_tilde: expanding '~' or '~/'");
			snprintf(expanded, sizeof(expanded), "%s%s", home, (*target) + 1);
			*target = expanded;
			LOG("expand_tilde: expanded='%s'", *target);
		}
		else
		{
			// "~user" (optional, skip for now)
			// For now, treat "~user" as literal.
			LOG("expand_tilde: '~user' form detected, leaving literal");
		}
	}

	LOG("expand_tilde: final='%s'", *target);
}

void terminal_process(input_state *st, const char *cmd)
{
	LOG("terminal_process: cmd='%s'", cmd);

	char buf[PATH_MAX];
	char *argv[MAX_ARGS];
	int argc;

	snprintf(buf, sizeof(buf), "%s", cmd);
	LOG("terminal_process: buf='%s'", buf);

	argc = input_parse_args(buf, argv, MAX_ARGS);
	LOG("terminal_process: argc=%d", argc);

	if (argc == 0)
	{
		LOG("terminal_process: argc==0, abort");
		return;
	}

	// always clear before executing a command
	LOG("terminal_process: clearing scroll and buffer");
	terminal_clear_scroll();

	LOG("terminal_process: adding to history");
	history_add(&st->hist, cmd);

	LOG("terminal_process: saving history");
	history_save(&st->hist);

	st->buf[0] = '\0';
	st->len = 0;
	st->cursor = 0;
	input_redraw(st, false);
	LOG("terminal_process: redraw input");

	terminal_cursor_line = terminal_scroll_top;
	LOG("terminal_process: cursor_line reset to %d", terminal_cursor_line);

	LOG("terminal_process: trying builtin");
	if (builtin_try(argc, argv))
	{
		LOG("terminal_process: builtin executed");
		input_cursor_redraw(st);
		return;
	}

	LOG("terminal_process: running external command");
	external_run(argv);

	LOG("terminal_process: external done");

	input_cursor_redraw(st);
	return;
}