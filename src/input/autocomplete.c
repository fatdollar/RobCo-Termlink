#include "main.h"
#include "autocomplete.h"
#include "input.h"
#include "terminal.h"
#include "history.h"
#include "util.h"

autocomplete_entry input_autocomplete_table[MAX_COMPLETIONS];
int input_autocomplete_count = 0;

static int cmp_match(const void *a, const void *b)
{
	const char *sa = (const char *)a;
	const char *sb = (const char *)b;
	return strcmp(sa, sb);
}

void input_autocomplete_path(char **argv, size_t *len, char *buf, autocomplete_type type)
{
	const char *prefix = (argv[1]) ? argv[1] : "";

	for (size_t i = 0; prefix[i]; i++)
		if ((unsigned char)prefix[i] < 32)
			((char *)prefix)[i] = '\0';

	/* Tilde expansion */
	if (prefix[0] == '~')
	{
		const char *home = getenv("HOME");
		if (!home)
			home = "/";

		static char expanded[PATH_MAX];

		if (prefix[1] == '/' || prefix[1] == '\0')
		{
			snprintf(expanded, sizeof(expanded), "%s%s", home, prefix + 1);
			prefix = expanded;
		}
	}

	char dirname[PATH_MAX] = {0};
	char partial[PATH_MAX] = {0};

	/* ============================================================
	   Compute dirname + partial from prefix
	   ============================================================ */
	if (prefix[0] == '/')
	{
		char *slash = strrchr(prefix, '/');

		if (slash == prefix)
		{
			snprintf(dirname, sizeof(dirname), "/");
			snprintf(partial, sizeof(partial), "%s", prefix + 1);
		}
		else
		{
			size_t dlen = (size_t)(slash - prefix);
			if (dlen >= sizeof(dirname))
				dlen = sizeof(dirname) - 1;

			memcpy(dirname, prefix, dlen);
			dirname[dlen] = '\0';

			snprintf(partial, sizeof(partial), "%s", slash + 1);
		}
	}
	else
	{
		char *slash = strrchr(prefix, '/');
		if (slash)
		{
			size_t dlen = (size_t)(slash - prefix);
			if (dlen >= sizeof(dirname))
				dlen = sizeof(dirname) - 1;

			memcpy(dirname, prefix, dlen);
			dirname[dlen] = '\0';

			snprintf(partial, sizeof(partial), "%s", slash + 1);
		}
		else
		{
			if (!getcwd(dirname, sizeof(dirname)))
				snprintf(dirname, sizeof(dirname), ".");

			snprintf(partial, sizeof(partial), "%s", prefix);
		}
	}

	DIR *d = opendir(dirname);
	if (!d)
		return;

	struct dirent *ent;
	char matches[128][PATH_MAX];
	int match_count = 0;

	/* ============================================================
	   Scan directory entries
	   ============================================================ */
	while ((ent = readdir(d)) != NULL)
	{
		size_t plen = strlen(partial);
		if (strncmp(ent->d_name, partial, plen) != 0)
			continue;

		if (plen > 0 &&
			(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0))
			continue;

		char fullpath[PATH_MAX];
		size_t dlen = strlen(dirname);
		size_t nlen = strlen(ent->d_name);

		if (dlen + 1 + nlen >= PATH_MAX)
			continue;

		memcpy(fullpath, dirname, dlen);
		fullpath[dlen] = '/';
		memcpy(fullpath + dlen + 1, ent->d_name, nlen);
		fullpath[dlen + 1 + nlen] = '\0';

		struct stat st;
		if (stat(fullpath, &st) != 0)
			continue;

		if (type == AUTOCOMPLETE_DIRS && !S_ISDIR(st.st_mode))
			continue;
		if (type == AUTOCOMPLETE_FILES && !S_ISREG(st.st_mode))
			continue;

		snprintf(matches[match_count], PATH_MAX, "%s", ent->d_name);
		matches[match_count][PATH_MAX - 1] = '\0';

		match_count++;
		if (match_count >= 128)
			break;
	}
	closedir(d);

	if (match_count == 0)
		return;

	/* Sort matches */
	qsort(matches, match_count, PATH_MAX, cmp_match);

	/* ============================================================
	   Single match → complete it
	   ============================================================ */
	if (match_count == 1)
	{
		const char *m = matches[0];

		size_t plen = strlen(partial);
		size_t mlen = strlen(m);

		if (*len >= plen)
			*len -= plen;
		buf[*len] = '\0';

		if (*len + mlen < PATH_MAX)
		{
			memcpy(buf + *len, m, mlen);
			*len += mlen;
			buf[*len] = '\0';
		}

		char fullpath[PATH_MAX];
		size_t dlen = strlen(dirname);
		size_t nlen = strlen(m);

		if (dlen + 1 + nlen < PATH_MAX)
		{
			memcpy(fullpath, dirname, dlen);
			fullpath[dlen] = '/';
			memcpy(fullpath + dlen + 1, m, nlen);
			fullpath[dlen + 1 + nlen] = '\0';

			struct stat st;
			if (stat(fullpath, &st) == 0 && S_ISDIR(st.st_mode))
			{
				if (*len + 1 < PATH_MAX)
				{
					buf[*len] = '/';
					(*len)++;
					buf[*len] = '\0';
				}
			}
		}

		return;
	}

	/* ============================================================
	   Multiple matches → print list
	   ============================================================ */
	terminal_slow_print("Matches:");
	for (int i = 0; i < match_count; i++)
		terminal_slow_print(matches[i]);
}

void input_autocomplete_tab(input_state *st)
{
	char temp[PATH_MAX];
	char *argv[MAX_ARGS];
	char *buf = st->buf;
	size_t *len = &st->len;

	/* Clear scroll region for match printing */
	terminal_clear_scroll();
	terminal_cursor_line = terminal_scroll_top;

	/* Parse arguments from buffer */
	/* Copy only up to cursor for parsing */
	snprintf(temp, sizeof(temp), "%.*s", (int)st->cursor, st->buf);
	int argc = input_parse_args(temp, argv, MAX_ARGS);
	if (argc == 0)
		return;

	/* Determine completion type from command */
	autocomplete_type type = AUTOCOMPLETE_FILES_AND_DIRS;
	autocomplete_entry *ce = input_autocomplete_lookup(argv[0]);
	if (ce)
		type = ce->type;

	/* ============================================================
	   Determine which argument to complete
	   Rules:
		 - If only command: complete empty second arg
		 - If flags present: complete LAST non-flag argument
		 - Else: complete argv[1]
	   ============================================================ */

	int target_index = 1;

	if (argc == 1)
	{
		/* "ls<TAB>" → complete empty second arg */
		argv[1] = "";
		target_index = 1;
	}
	else
	{
		/* If argv[1] is a flag, complete the last argument */
		if (argv[1][0] == '-' && argc > 2)
		{
			target_index = argc - 1;
		}
		else
		{
			/* Normal case: complete argv[1] */
			target_index = 1;
		}
	}

	/* Build a minimal argv view for completion */
	char *comp_argv[2];
	comp_argv[0] = argv[0];
	comp_argv[1] = argv[target_index];

	/* Save old buffer for change detection */
	char oldbuf[PATH_MAX];
	snprintf(oldbuf, sizeof(oldbuf), "%s", buf);

	/* ===== Save suffix after cursor and limit completion to prefix ===== */
	char suffix[PATH_MAX];
	snprintf(suffix, sizeof(suffix), "%s", buf + st->cursor);

	size_t original_len = *len;
	*len = st->cursor;
	buf[*len] = '\0';
	/* ======================================================================= */

	/* Perform completion */
	input_autocomplete_path(comp_argv, len, buf, type);

	/* ============================================================
	   Redraw ONLY if buffer changed
	   ============================================================ */
	if (strcmp(oldbuf, buf) != 0)
	{
		/* ===== NEW: reattach suffix after completed prefix ===== */
		size_t prefix_len = *len;
		size_t suffix_len = strlen(suffix);

		if (prefix_len + suffix_len < PATH_MAX)
		{
			memcpy(buf + prefix_len, suffix, suffix_len + 1); /* +1 for '\0' */
			*len = prefix_len + suffix_len;
		}
		else
		{
			memcpy(buf + prefix_len, suffix, PATH_MAX - prefix_len - 1);
			*len = PATH_MAX - 1;
			buf[*len] = '\0';
		}

		/* After completion, cursor should be at end of completed prefix */
		st->cursor = prefix_len;
		/* ======================================================== */

		printf("\033[%d;%dH", terminal_rows, PROMPT_COL);
		printf("\033[K");
		for (size_t i = 0; i < *len; i++)
			terminal_slow_putc(buf[i]);

		printf("\033[%d;%dH", terminal_rows, PROMPT_COL + (int)st->cursor);
		fflush(stdout);
	}
	else
	{
		/* restore len if no change */
		*len = original_len;
		buf[*len] = '\0';
	}
}

autocomplete_entry *input_autocomplete_lookup(const char *cmd)
{
	for (int i = 0; i < input_autocomplete_count; i++)
	{
		if (strcmp(input_autocomplete_table[i].cmd, cmd) == 0)
			return &input_autocomplete_table[i];
	}
	return NULL;
}