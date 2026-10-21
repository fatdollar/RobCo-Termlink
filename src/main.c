#include "main.h"
#include "autocomplete.h"
#include "terminal.h"
#include "input.h"
#include "builtin.h"
#include "external.h"
#include "theme.h"
#include "history.h"
#include "tl.h"

/* ============================================================
 * GLOBAL STATE
 * ============================================================ */

extern char **environ;

tl_exec_ctx_t tl_ctx = {
	.posix_mode = false,
	.termlink_mode = true,
	.interactive = true,
	.last_status = 0,
	.output_fn = terminal_slow_print,
	.error_fn = terminal_slow_print};

/* ============================================================
 * MAIN
 * ============================================================ */

int main(void)
{
	char input_buf[PATH_MAX];
	input_state st;
	memset(&st, 0, sizeof(st));

	tl_ctx.envp = environ;

#ifdef TERMLINK_DEBUG
	debug_init();
#endif

	// Try to become session leader
	if (setsid() >= 0)
	{
		// Try to acquire controlling terminal
		if (ioctl(STDIN_FILENO, TIOCSCTTY, 0) >= 0)
		{
			termlink_job_control_enabled = 1;
			LOG("Job control enabled (session leader)");
		}
		else
		{
			LOG("TIOCSCTTY failed — fallback mode");
		}
	}
	else
	{
		LOG("setsid failed — fallback mode");
	}

	terminal_setup();

	history_load(&st.hist);
	input_autocomplete_load_completions();
	builtin_init();
	external_load_from_path();

	terminal_prompt_draw();
	input_redraw(&st, true);

	while (1)
	{
		input_read_line(&st, input_buf, sizeof(input_buf));
		terminal_process(&st, input_buf);
	}

	return 0;
}