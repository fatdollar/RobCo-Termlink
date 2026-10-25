#include "main.h"
#include "terminal.h"
#include "builtin.h"
#include "external.h"
#include "theme.h"
#include "input.h"
#include "cursor.h"
#include "tl.h"

/* ============================================================
 * GLOBAL STATE
 * ============================================================ */

struct termios terminal_orig_termios;

scroll_line scroll_lines[MAX_SCROLL_LINES];
int scroll_line_count;

int terminal_rows = 24;
int terminal_cols = 80;

int terminal_scroll_top = 5;
int terminal_scroll_bottom = 0;
int terminal_cursor_line = 5;

int termlink_job_control_enabled = 0;

/* Track pending resizes asynchronously */
static volatile sig_atomic_t terminal_resize_flag = 0;

bool termlink_is_login_shell = false;

/* ============================================================
 * FORWARD DECLARATIONS
 * ============================================================ */

static void _terminal_handle_sigwinch(int sig);
static void _terminal_slow_putc(char c);
static void _terminal_med_putc(char c);
static void _terminal_putc(char c);
static int _terminal_start_col(int line, const char *s);
static int _terminal_visible_len(const char *s, int start_col);
static void _terminal_restore(void);
static void _terminal_update_size(void);

/* ============================================================
 * INTERNAL HELPERS
 * ============================================================ */

static void _terminal_handle_sigwinch(int sig)
{
	(void)sig;
	terminal_resize_flag = 1;
}

static void _terminal_slow_putc(char c)
{
	putchar(c);
	fflush(stdout);
	usleep(PRINT_DELAY);
}

static void _terminal_med_putc(char c)
{
	putchar(c);
	fflush(stdout);
	usleep(PRINT_DELAY / 4);
}

static void _terminal_putc(char c)
{
	putchar(c);
	fflush(stdout);
	usleep(PRINT_DELAY / 4);
}

static int _terminal_start_col(int line, const char *s)
{
	// Move cursor to column 1 ON THE TARGET LINE
	printf("\033[%d;1H", line);
	fflush(stdout);

	// Print ONLY leading whitespace
	const char *p = s;
	while (*p == ' ' || *p == '\t')
	{
		LOG("_terminal_start_col: *p=%c", *p);
		_terminal_slow_putc(*p);
		p++;
	}

	// Query actual cursor column
	int col = input_cursor_get_col();
	if (col < 1)
		col = 1;

	return col;
}

static int _terminal_visible_len(const char *s, int start_col)
{
	// Cursor is already at end of leading whitespace
	const char *p = s;
	while (*p == ' ' || *p == '\t')
		p++;

	// Print ONLY visible characters
	for (; *p; ++p)
		_terminal_slow_putc(*p);

	// Query final cursor column
	int final_col = input_cursor_get_col();
	if (final_col < start_col)
		final_col = start_col;

	return final_col - start_col;
}

static void _terminal_restore(void)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &terminal_orig_termios);
}

static void _terminal_update_size(void)
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

/* ============================================================
 * DIMENSIONS / INPUT
 * ============================================================ */

int terminal_scroll_rows(void)
{
	return terminal_scroll_bottom - terminal_scroll_top;
}

int terminal_scroll_cols(void)
{
	return terminal_cols;
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

/* ============================================================
 * LOGIN
 * ============================================================ */

void termlink_check_login(int argc, char **argv)
{
	// Check if argv[0] starts with '-' (e.g., "-termlink")
	if (argv[0] && argv[0][0] == '-')
	{
		termlink_is_login_shell = true;
	}

	// Or check explicit flags
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--login") == 0)
		{
			termlink_is_login_shell = true;
			break;
		}
	}

	if (termlink_is_login_shell)
	{
		LOG("login: identified as a login shell");
	}
}

void termlink_init_login_profiles(void)
{
	if (!termlink_is_login_shell)
		return;

	LOG("login: sourcing initialization profiles");

	// 1. System-wide profile
	if (access("/etc/profile", R_OK) == 0)
	{
		LOG("login: sourcing /etc/profile");
		char *args[] = {".", "/etc/profile", NULL};
		// Assuming your source builtin can be called programmatically:
		// builtin_source(2, args);
	}

	// 2. User profile (~/.profile or ~/.termlink_profile)
	const char *home = getenv("HOME");
	if (home)
	{
		char profile_path[PATH_MAX];
		snprintf(profile_path, sizeof(profile_path), "%s/.profile", home);
		if (access(profile_path, R_OK) == 0)
		{
			LOG("login: sourcing %s", profile_path);
			char *args[] = {".", profile_path, NULL};
			// builtin_source(2, args);
		}
	}
}

/* ============================================================
 * SETUP
 * ============================================================ */

void terminal_setup(void)
{
	struct termios t;

	LOG("terminal_setup: entering raw mode");
	tcgetattr(STDIN_FILENO, &terminal_orig_termios);
	atexit(_terminal_restore);

	t = terminal_orig_termios;
	t.c_lflag &= ~(ICANON | ECHO);
	t.c_lflag |= ISIG;
	t.c_cc[VMIN] = 1;
	t.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSANOW, &t);

	if (isatty(STDIN_FILENO))
	{
		pid_t shell_pgid = getpid();
		if (setpgid(shell_pgid, shell_pgid) < 0)
		{
			LOG("terminal_setup: setpgid failed");
		}
		tcsetpgrp(STDIN_FILENO, shell_pgid);

		signal(SIGINT, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		signal(SIGTSTP, SIG_IGN);
		signal(SIGTTIN, SIG_IGN);
		signal(SIGTTOU, SIG_IGN);
		signal(SIGWINCH, _terminal_handle_sigwinch);

		LOG("terminal_setup: job control active (interactive tty)");
	}
	else
	{
		LOG("terminal_setup: non-interactive mode (stdin redirected)");
	}

	_terminal_update_size();

	theme_load_config();
	LOG("terminal_setup: rows=%d cols=%d", terminal_rows, terminal_cols);

	terminal_header_print();

	terminal_slow_print_at(terminal_rows, "Accessing systems...");
}

/* ============================================================
 * SCREEN RESIZE
 * ============================================================ */

bool terminal_check_resize(void)
{
	if (terminal_resize_flag)
	{
		terminal_resize_flag = 0;
		_terminal_update_size();
		return true;
	}

	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0)
	{
		// Check if dimensions changed compared to active globals
		if (ws.ws_row != terminal_rows || ws.ws_col != terminal_cols)
		{
			_terminal_update_size(); // This updates terminal_rows, terminal_cols, and scroll bounds
			return true;
		}
	}
	return false;
}

/* ============================================================
 * PRINTING
 * ============================================================ */

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
		{
			if (*p == ' ' || *p == '\t')
				_terminal_putc(*p); // FAST PRINT
			else
				_terminal_slow_putc(*p); // SLOW PRINT
		}

		return;
	}

	// SCROLL REGION — USE start_col logic
	int start_col = _terminal_start_col(line, s);
	int visible_len = _terminal_visible_len(s, start_col);

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

void terminal_scroll_print(int row, const char *text)
{
	terminal_slow_print_at(terminal_scroll_top + row, text);
}

void terminal_scroll_print_highlighted(int row, const char *text)
{
	char buf[512];
	snprintf(buf, sizeof(buf), "\033[7m%s\033[0m", text);
	terminal_scroll_print(row, buf);
}

/* ============================================================
 * CLEAR / HEADER / PROMPT
 * ============================================================ */

void terminal_scroll_clear(void)
{
	int prompt_row = terminal_rows; // bottom row

	LOG("terminal_scroll_clear: slow-clearing %d lines", scroll_line_count);

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
				_terminal_med_putc(' ');
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
	LOG("terminal_scroll_clear: cursor_line=%d", terminal_cursor_line);
}

void terminal_prompt_draw(void)
{
	LOG("terminal_draw_prompt: moving cursor to bottom row %d", terminal_rows);
	printf("\033[%d;1H", terminal_rows);

	terminal_cursor_line = terminal_scroll_top;

	LOG("terminal_draw_prompt: printing prompt '>'");
	putchar('>');
}

void terminal_header_print(void)
{
	LOG("terminal_print_header: clearing screen");
	printf("\033[2J\033[H");

	LOG("terminal_print_header: printing header");

	// Build full-width bar
	int width = terminal_cols;
	char bar[PATH_MAX];
	memset(bar, '=', width);
	bar[width] = '\0';

	// Build centered title
	const char *title = "RobCo Industries (TM) Termlink v1.0";
	int tlen = strlen(title);
	int pad = (width - tlen) / 2;
	if (pad < 0)
		pad = 0;

	char centered[PATH_MAX];
	memset(centered, ' ', width);
	memcpy(centered + pad, title, tlen);
	centered[width] = '\0';

	// Print header
	terminal_slow_print_at(1, bar);
	terminal_slow_print_at(2, centered);
	terminal_slow_print_at(3, bar);
	terminal_slow_print_at(4, "");

	LOG("terminal_print_header: clearing scroll region");
	terminal_scroll_clear();
}

/* ============================================================
 * PATH / TILDE
 * ============================================================ */

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

/* ============================================================
 * COMMAND PROCESSING
 * ============================================================ */

void terminal_process(input_state *st, const char *cmd)
{
	char *cmd_copy = strdup(cmd);

	LOG("terminal_process: cmd='%s'", cmd_copy);

	if (!cmd_copy || !*cmd_copy)
		return;

	// always clear before executing a command
	LOG("terminal_process: clearing scroll and buffer");
	terminal_scroll_clear();

	LOG("terminal_process: adding to history");
	history_add(&st->hist, cmd_copy);

	LOG("terminal_process: saving history");
	history_save(&st->hist);

	st->buf[0] = '\0';
	st->len = 0;
	st->cursor = 0;
	input_redraw(st, false);
	LOG("terminal_process: redraw input");

	terminal_cursor_line = terminal_scroll_top;
	LOG("terminal_process: cursor_line reset to %d", terminal_cursor_line);

	/* Phase 1: hand off to Termlink executor */
	tl_exec_line(cmd_copy, &tl_ctx);

	terminal_prompt_draw();
	LOG("terminal_process: exec done");

	input_cursor_redraw(st);
}