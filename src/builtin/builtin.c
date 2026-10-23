#include "main.h"
#include "builtin.h"
#include "terminal.h"
#include "history.h"
#include "input.h"
#include "input_state.h"

/* ============================================================
 *  COMMAND
 * ============================================================ */

void builtin_init(void)
{
	/* Core Utilities */
	builtin_register("cd", builtin_cd);
	builtin_register("clear", builtin_clear);
	builtin_register("exit", builtin_exit);
	builtin_register("help", builtin_help);
	builtin_register("export", builtin_export);
	builtin_register("unset", builtin_unset);
	builtin_register("pwd", builtin_pwd);
	builtin_register("echo", builtin_echo);
	builtin_register("type", builtin_type);
	builtin_register("set", builtin_set);
	builtin_register("shift", builtin_shift);
	builtin_register("read", builtin_read);
	builtin_register("umask", builtin_umask);
	builtin_register("alias", builtin_alias);
	builtin_register("unalias", builtin_unalias);
	builtin_register(".", builtin_source);
	builtin_register("source", builtin_source);
	builtin_register("trap", builtin_trap);
	builtin_register("getopts", builtin_getopts);
	builtin_register("jobs", builtin_jobs);
	builtin_register("fg", builtin_fg);
	builtin_register("bg", builtin_bg);
	builtin_register("kill", builtin_kill);
	builtin_register("wait", builtin_wait);

	/* Complete POSIX Additions */
	builtin_register("eval", builtin_eval);
	builtin_register("exec", builtin_exec);
	builtin_register(":", builtin_colon);
	builtin_register("true", builtin_true);
	builtin_register("false", builtin_false);
	builtin_register("command", builtin_command);
	builtin_register("readonly", builtin_readonly);
	builtin_register("return", builtin_return);
	builtin_register("break", builtin_break);
	builtin_register("continue", builtin_continue);
	builtin_register("times", builtin_times);
}