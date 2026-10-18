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

int _compute_start_col(int line, const char *s)
{
	// Move cursor to column 1 ON THE TARGET LINE
	printf("\033[%d;1H", line);
	fflush(stdout);

	// Print ONLY leading whitespace
	const char *p = s;
	while (*p == ' ' || *p == '\t')
	{
		LOG("_compute_start_col: *p=%c", *p);
		terminal_slow_putc(*p);
		p++;
	}

	// Query actual cursor column
	int col = input_cursor_get_col();
	if (col < 1)
		col = 1;

	return col;
}

int _compute_visible_len(int line, const char *s, int start_col)
{
	// Cursor is already at end of leading whitespace
	const char *p = s;
	while (*p == ' ' || *p == '\t')
		p++;

	// Print ONLY visible characters
	for (; *p; ++p)
		terminal_slow_putc(*p);

	// Query final cursor column
	int final_col = input_cursor_get_col();
	if (final_col < start_col)
		final_col = start_col;

	return final_col - start_col;
}

void terminal_sigint(int sig)
{
	LOG("terminal_sigint: sigint skipped for termlink");
}

int terminal_scroll_rows(void)
{
	return terminal_scroll_bottom - terminal_scroll_top;
}

bool terminal_poll_key(int *key)
{
	struct timeval tv = {0, 0};
	fd_set fds;

	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);

	int r = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
	if (r > 0 && FD_ISSET(STDIN_FILENO, &fds))
	{
		unsigned char c;
		if (read(STDIN_FILENO, &c, 1) == 1)
		{
			*key = c;
			return true;
		}
	}

	return false;
}

int terminal_scroll_cols(void)
{
	return terminal_cols;
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

void terminal_med_putc(char c)
{
	putchar(c);
	fflush(stdout);
	usleep(PRINT_DELAY / 4);
}

void terminal_slow_print_at(int line, const char *s)
{
	int prompt_row = terminal_rows;

	// HEADER OR PROMPT — DO NOT USE start_col logic
	if (line < terminal_scroll_top || line >= prompt_row)
	{
		LOG("slow_print_at: HEADER/PROMPT line=%d raw='%s'", line, s);

		printf("\033[%d;1H", line);
		fflush(stdout);

		for (const char *p = s; *p; ++p)
			terminal_slow_putc(*p);

		return;
	}

	// SCROLL REGION — USE start_col logic
	int start_col = _compute_start_col(line, s);
	int visible_len = _compute_visible_len(line, s, start_col);

	LOG("slow_print_at: SCROLL line=%d raw='%s' start_col=%d visible_len=%d",
		line, s, start_col, visible_len);

	if (scroll_line_count < MAX_SCROLL_LINES)
	{
		scroll_lines[scroll_line_count++] =
			(scroll_line){line, start_col, visible_len};

		LOG("slow_print_at: TRACKED row=%d start_col=%d len=%d",
			line, start_col, visible_len);
	}
}

void terminal_slow_print(const char *s)
{
	terminal_slow_print_at(terminal_cursor_line, s);
	terminal_cursor_line++;
}

void terminal_write_scroll_line(int row, const char *text)
{
	terminal_slow_print_at(terminal_scroll_top + row, text);
}

void terminal_write_status(const char *text)
{
	terminal_slow_print_at(terminal_scroll_bottom, text);
}

void terminal_clear_scroll(void)
{
	int prompt_row = terminal_rows; // bottom row

	LOG("terminal_clear_scroll: slow-clearing %d lines", scroll_line_count);

	// Slow-clear only lines that contain content
	for (int i = 0; i < scroll_line_count; i++)
	{
		int row = scroll_lines[i].row;
		int start_col = scroll_lines[i].start_col;
		int len = scroll_lines[i].length;

		if (row >= terminal_scroll_top && row < prompt_row)
		{
			printf("\033[%d;%dH", row, start_col);

			for (int c = 0; c < len; c++)
				terminal_med_putc(' ');
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
	terminal_slow_print_at(1, "==============================================================================");
	terminal_slow_print_at(2, "                      RobCo Industries (TM) Termlink v1.0");
	terminal_slow_print_at(3, "==============================================================================");
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

	LOG("terminal_process: checking external existence");
	if (external_exists(argv[0]))
	{
		LOG("terminal_process: external exists");

		if (external_is_tty_interactive(argv[0]))
		{
			LOG("terminal_process: external is TTY interactive");
			terminal_slow_print("Interactive TTY programs are not yet supported: ");
			terminal_slow_print(argv[0]);
			terminal_slow_print("\n");
		}
		else
		{
			LOG("terminal_process: running external command");
			external_run_paged(argv);
		}
	}
	else
	{
		LOG("terminal_process: unknown command");
		terminal_slow_print("Unknown command: ");
		terminal_slow_print(argv[0]);
		terminal_slow_print("\n");
	}

	terminal_draw_prompt();

	LOG("terminal_process: external done");

	input_cursor_redraw(st);
	return;
}