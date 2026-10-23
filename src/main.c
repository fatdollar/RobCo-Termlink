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

#ifdef DEBUG
	debug_init();
#endif

	// Enable job control if running interactively on a terminal
	if (isatty(STDIN_FILENO))
	{
		termlink_job_control_enabled = 1;

		// Attempt to become session leader / acquire controlling terminal if possible
		if (setsid() >= 0)
		{
			ioctl(STDIN_FILENO, TIOCSCTTY, 0);
			LOG("Job control enabled (session leader)");
		}
		else
		{
			LOG("Job control enabled (standard interactive mode)");
		}
	}
	else
	{
		termlink_job_control_enabled = 0;
		LOG("Non-interactive mode — fallback mode");
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