#include "main.h"
#include "terminal.h"
#include "input.h"
#include "builtin.h"
#include "external.h"
#include "util.h"
#include "theme.h"
#include "history.h"
#include "read_line.h"

int main(void)
{
	char input_buf[PATH_MAX];
	input_state st;
	memset(&st, 0, sizeof(st));

#ifdef TERMLINK_DEBUG
	debug_init();
#endif

	struct sigaction sa;
	sa.sa_handler = terminal_sigint;
	sigaction(SIGINT, &sa, NULL);

	terminal_setup();
	terminal_update_size();
	terminal_print_header();

	terminal_slow_print_at(terminal_rows, "Accessing system...");

	history_load(&st.hist);
	util_load_completions();
	builtin_init();
	external_load_from_path();

	terminal_draw_prompt();
	input_redraw(&st, true);

	while (1)
	{
		input_read_line(&st, input_buf, sizeof(input_buf));
		terminal_process(&st, input_buf);
	}

	return 0;
}