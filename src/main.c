#include "main.h"
#include "terminal.h"
#include "input.h"
#include "builtin.h"
#include "util.h"
#include "theme.h"
#include "history.h"
#include "read_line.h"
#ifdef TERMLINK_DEBUG
#include "debug.h"
#endif

int main(void)
{
	char input_buf[PATH_MAX];
	input_state st;
	memset(&st, 0, sizeof(st));

#ifdef TERMLINK_DEBUG
	debug_init();
#endif

	terminal_setup();
	terminal_update_size();
	terminal_print_header();

	terminal_slow_print_at(terminal_rows, "Accessing system...");

	history_load(&st.hist);
	util_load_completions();
	builtin_load_external_from_path();

	terminal_draw_prompt();

	while (1)
	{
		input_read_line(&st, input_buf, sizeof(input_buf));
		builtin_process(&st, input_buf);
	}

	return 0;
}