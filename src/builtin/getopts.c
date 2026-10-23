#include "main.h"
#include "builtin.h"
#include "terminal.h"

/* ============================================================
 * GLOBAL STATE & EXTERNS
 * ============================================================ */

extern char *positional_params[];
extern int positional_count;

static int optind_internal = 1;
static char *optarg_internal = NULL;
static int optopt_internal = 0;

/* ============================================================
 *  INTERNAL HELPERS
 * ============================================================ */

static void _builtin_getopts_sync_env(void)
{
	char buf[32];

	snprintf(buf, sizeof(buf), "%d", optind_internal);
	setenv("OPTIND", buf, 1);

	if (optarg_internal)
		setenv("OPTARG", optarg_internal, 1);
	else
		unsetenv("OPTARG");

	snprintf(buf, sizeof(buf), "%d", optopt_internal);
	setenv("OPTOPT", buf, 1);
}

/* ============================================================
 *  COMMAND
 * ============================================================ */

int builtin_getopts(int argc, char **argv)
{
	LOG("getopts: argc=%d", argc);

	if (argc < 3)
	{
		terminal_slow_print("getopts: usage: getopts optstring name\n");
		return 1;
	}

	const char *optstring = argv[1];
	const char *name = argv[2];

	/* Load OPTIND from environment if present (1-based index into positional parameters) */
	const char *env_optind = getenv("OPTIND");
	if (env_optind)
		optind_internal = atoi(env_optind);
	else
		optind_internal = 1;

	LOG("getopts: OPTIND=%d, positional_count=%d", optind_internal, positional_count);

	/* Check if we've exhausted positional parameters (OPTIND is 1-based, so index is optind_internal - 1) */
	if (optind_internal > positional_count || positional_params[optind_internal - 1] == NULL)
	{
		setenv(name, "?", 1);
		_builtin_getopts_sync_env();
		return 1; // End of options (non-zero exit status)
	}

	char *arg = positional_params[optind_internal - 1];

	/* Must begin with '-' and not be empty or "--" */
	if (arg == NULL || arg[0] != '-' || arg[1] == '\0')
	{
		setenv(name, "?", 1);
		_builtin_getopts_sync_env();
		return 1;
	}

	if (strcmp(arg, "--") == 0)
	{
		// Explicit end of options marker; consume it and terminate
		optind_internal++;
		setenv(name, "?", 1);
		_builtin_getopts_sync_env();
		return 1;
	}

	char opt = arg[1];
	optopt_internal = opt;

	LOG("getopts: option='%c'", opt);

	/* Find option in optstring */
	const char *p = strchr(optstring, opt);
	if (!p)
	{
		/* Unknown option */
		if (optstring[0] == ':')
			setenv(name, ":", 1);
		else
			setenv(name, "?", 1);

		optind_internal++;
		_builtin_getopts_sync_env();
		return 1;
	}

	/* Check if option requires an argument */
	if (*(p + 1) == ':')
	{
		if (arg[2] != '\0')
		{
			/* Argument is attached: -oVALUE */
			if (optarg_internal)
				free(optarg_internal);
			optarg_internal = strdup(&arg[2]);
		}
		else if (optind_internal < positional_count)
		{
			/* Argument is the next positional parameter */
			optind_internal++;
			if (optarg_internal)
				free(optarg_internal);
			optarg_internal = strdup(positional_params[optind_internal - 1]);
		}
		else
		{
			/* Missing argument */
			if (optstring[0] == ':')
				setenv(name, ":", 1);
			else
				setenv(name, "?", 1);

			optind_internal++;
			_builtin_getopts_sync_env();
			return 1;
		}
	}
	else
	{
		/* No argument required */
		if (optarg_internal)
		{
			free(optarg_internal);
			optarg_internal = NULL;
		}
	}

	/* Set the variable name to the option character */
	char buf[2] = {opt, '\0'};
	setenv(name, buf, 1);

	optind_internal++;
	_builtin_getopts_sync_env();

	return 0; // Success (POSIX requires 0 exit status when an option is matched)
}