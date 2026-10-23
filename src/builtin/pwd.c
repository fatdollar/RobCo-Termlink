#include "main.h"
#include "builtin.h"
#include "terminal.h"

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_pwd(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	char buf[PATH_MAX];

	LOG("pwd: called");

	if (getcwd(buf, sizeof(buf)) != NULL)
	{
		LOG("pwd: getcwd='%s'", buf);
		terminal_slow_print(buf);
		terminal_slow_print("\n");
		return 0; // Success
	}

	const char *pwd = getenv("PWD");
	LOG("pwd: getcwd failed, PWD='%s'", pwd ? pwd : "(null)");

	if (pwd)
	{
		terminal_slow_print(pwd);
		terminal_slow_print("\n");
		return 0; // Success
	}

	terminal_slow_print("pwd: cannot determine current directory\n");
	return 1;
}