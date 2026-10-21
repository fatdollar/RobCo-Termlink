#include "main.h"
#include "terminal.h"
#include "pager.h"
#include "external.h"
#include "builtin.h"

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_help(int argc, char **argv)
{
	LOG("builtin_help: generating help output");

	pager p;
	screen_pager_start(&p);

	pager_print_line(&p, "Available commands:");
	pager_print_line(&p, "");
	pager_print_line(&p, "Built-ins:");
	pager_print_line(&p, "  help");
	pager_print_line(&p, "  clear");
	pager_print_line(&p, "  exit");
	pager_print_line(&p, "  cd");
	pager_print_line(&p, "  export");
	pager_print_line(&p, "  unset");
	pager_print_line(&p, "");

	pager_print_line(&p, "External commands:");

	for (int i = 0; i < external_cmd_count; i++)
	{
		char buf[256];
		snprintf(buf, sizeof(buf), "  %.240s", external_cmds[i]);
		pager_print_line(&p, buf);

		LOG("builtin_help: listed external '%s'", external_cmds[i]);
	}

	screen_pager_finish(&p);
	screen_pager_run(&p);
	screen_pager_free(&p);

	return 1;
}