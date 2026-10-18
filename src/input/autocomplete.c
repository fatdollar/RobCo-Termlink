#include "main.h"
#include "input.h"
#include "autocomplete.h"
#include "terminal.h"
#include "history.h"
#include "pager.h"

/* ============================================================
 * GLOBAL STATE
 * ============================================================ */

autocomplete_entry input_autocomplete_table[MAX_COMPLETIONS];
int input_autocomplete_count = 0;

/* ============================================================
 * INTERNAL HELPERS (static)
 * ============================================================ */

static int _cmp_match(const void *a, const void *b)
{
	LOG("cmp_match: '%s' vs '%s'", (char *)a, (char *)b);
	return strcmp((const char *)a, (const char *)b);
}

static autocomplete_type _autocomplete_parse_completion_type(const char *value)
{
	LOG("_autocomplete_parse_completion_type: value='%s'", value);

	if (strcmp(value, "dirs") == 0)
		return AUTOCOMPLETE_DIRS;
	if (strcmp(value, "files") == 0)
		return AUTOCOMPLETE_FILES;
	if (strcmp(value, "files_and_dirs") == 0)
		return AUTOCOMPLETE_FILES_AND_DIRS;
	if (strcmp(value, "executables") == 0)
		return AUTOCOMPLETE_EXECUTABLES;
	if (strcmp(value, "dynamic") == 0)
		return AUTOCOMPLETE_DYNAMIC;

	LOG("_autocomplete_parse_completion_type: default");
	return AUTOCOMPLETE_FILES_AND_DIRS;
}

static void _autocomplete_register_completion(char *cmd, autocomplete_type type)
{
	LOG("_autocomplete_register_completion: cmd='%s' type=%d", cmd, type);

	if (input_autocomplete_count >= MAX_COMPLETIONS)
	{
		LOG("_autocomplete_register_completion: MAX_COMPLETIONS reached");
		return;
	}

	input_autocomplete_table[input_autocomplete_count].cmd = cmd;
	input_autocomplete_table[input_autocomplete_count].type = type;
	input_autocomplete_count++;

	LOG("_autocomplete_register_completion: count=%d", input_autocomplete_count);
}

static autocomplete_entry *_input_autocomplete_lookup(const char *cmd)
{
	LOG("autocomplete_lookup: cmd='%s'", cmd);

	for (int i = 0; i < input_autocomplete_count; i++)
	{
		LOG("autocomplete_lookup: checking '%s'",
			input_autocomplete_table[i].cmd);

		if (strcmp(input_autocomplete_table[i].cmd, cmd) == 0)
		{
			LOG("autocomplete_lookup: match");
			return &input_autocomplete_table[i];
		}
	}

	LOG("autocomplete_lookup: no match");
	return NULL;
}

void _autocomplete_load_completions_dir(const char *dirpath)
{
	LOG("_autocomplete_load_completions_dir: dir='%s'", dirpath);

	DIR *d = opendir(dirpath);
	if (!d)
	{
		LOG("_autocomplete_load_completions_dir: cannot open");
		return;
	}

	struct dirent *ent;
	while ((ent = readdir(d)) != NULL)
	{
		LOG("_autocomplete_load_completions_dir: found '%s'", ent->d_name);

		const char *name = ent->d_name;
		size_t len = strlen(name);

		if (len <= 5)
			continue;
		if (strcmp(name + len - 5, ".comp") != 0)
			continue;

		LOG("_autocomplete_load_completions_dir: .comp file '%s'", name);

		char cmd[64];
		size_t cmdlen = len - 5;
		if (cmdlen >= sizeof(cmd))
			cmdlen = sizeof(cmd) - 1;
		memcpy(cmd, name, cmdlen);
		cmd[cmdlen] = '\0';

		LOG("_autocomplete_load_completions_dir: cmd='%s'", cmd);

		char path[PATH_MAX];
		snprintf(path, sizeof(path), "%s/%s", dirpath, name);

		FILE *f = fopen(path, "r");
		if (!f)
		{
			LOG("_autocomplete_load_completions_dir: cannot open file '%s'", path);
			continue;
		}

		/* Defaults */
		char line[256];
		autocomplete_type type = AUTOCOMPLETE_FILES_AND_DIRS;
		const char *dynamic_prog = NULL;

		/* Temporary storage for options/extensions */
		static char *options[64];
		static char *extensions[64];
		int opt_count = 0;
		int ext_count = 0;

		bool in_options_block = false;

		while (fgets(line, sizeof(line), f))
		{
			LOG("_autocomplete_load_completions_dir: line='%s'", line);

			char *nl = strchr(line, '\n');
			if (nl)
				*nl = '\0';

			/* Skip comments */
			if (line[0] == '#' || line[0] == '\0')
				continue;

			/* command: NAME */
			if (strncmp(line, "command:", 8) == 0)
			{
				/* ignore, filename already gives command */
				continue;
			}

			/* type: VALUE */
			if (strncmp(line, "type:", 5) == 0)
			{
				const char *val = line + 5;
				while (*val == ' ' || *val == '\t')
					val++;

				type = _autocomplete_parse_completion_type(val);
				LOG("_autocomplete_load_completions_dir: parsed type=%d", type);
				continue;
			}

			/* extensions: ext1, ext2, ext3 */
			if (strncmp(line, "extensions:", 11) == 0)
			{
				char *p = line + 11;
				while (*p == ' ' || *p == '\t')
					p++;

				char *tok = strtok(p, ",");
				while (tok && ext_count < 63)
				{
					while (*tok == ' ' || *tok == '\t')
						tok++;
					extensions[ext_count++] = strdup(tok);
					tok = strtok(NULL, ",");
				}
				extensions[ext_count] = NULL;

				LOG("_autocomplete_load_completions_dir: parsed %d extensions", ext_count);
				continue;
			}

			/* options: (start block) */
			if (strncmp(line, "options:", 8) == 0)
			{
				in_options_block = true;
				continue;
			}

			/* options block entries: "  - something" */
			if (in_options_block && strncmp(line, "  - ", 4) == 0)
			{
				if (opt_count < 63)
				{
					options[opt_count++] = strdup(line + 4);
					LOG("_autocomplete_load_completions_dir: option='%s'", line + 4);
				}
				continue;
			}

			/* dynamic: helper_program */
			if (strncmp(line, "dynamic:", 8) == 0)
			{
				const char *val = line + 8;
				while (*val == ' ' || *val == '\t')
					val++;
				dynamic_prog = strdup(val);
				LOG("_autocomplete_load_completions_dir: dynamic='%s'", dynamic_prog);
				continue;
			}
		}

		fclose(f);

		/* Register command */
		_autocomplete_register_completion(cmd, type);

		/* Fill entry fields */
		autocomplete_entry *e = _input_autocomplete_lookup(cmd);
		if (e)
		{
			e->options = (opt_count > 0) ? (const char **)options : NULL;
			e->extensions = (ext_count > 0) ? (const char **)extensions : NULL;
			e->dynamic_prog = dynamic_prog;
		}
	}

	closedir(d);
}

static void _input_autocomplete_command(const char *prefix, size_t *len, char *buf)
{
	LOG("autocomplete_cmd: prefix='%s' len=%zu buf='%s'", prefix, *len, buf);

	/* Strip control chars from prefix */
	char clean[PATH_MAX];
	snprintf(clean, sizeof(clean), "%s", prefix);
	for (size_t i = 0; clean[i]; i++)
		if ((unsigned char)clean[i] < 32)
			clean[i] = '\0';

	LOG("autocomplete_cmd: sanitized prefix='%s'", clean);

	/* Lookup completion entry */
	autocomplete_entry *ce = _input_autocomplete_lookup(clean);
	autocomplete_type type = AUTOCOMPLETE_EXECUTABLES;

	if (ce)
	{
		type = ce->type;
		LOG("autocomplete_cmd: using type=%d from .comp", type);
	}

	const char *path = getenv("PATH");
	if (!path || !*path)
	{
		LOG("autocomplete_cmd: PATH is empty");
		return;
	}

	char matches[128][PATH_MAX];
	int match_count = 0;

	/* Add built-in commands */
	for (int i = 0; i < input_autocomplete_count && match_count < 128; i++)
	{
		const char *cmd = input_autocomplete_table[i].cmd;
		size_t plen = strlen(clean);

		if (strncmp(cmd, clean, plen) == 0)
		{
			snprintf(matches[match_count], PATH_MAX, "%s", cmd);
			matches[match_count][PATH_MAX - 1] = '\0';
			match_count++;
		}
	}

	/* Walk PATH components */
	const char *p = path;
	while (*p && match_count < 128)
	{
		char dir[PATH_MAX];
		size_t dlen = 0;

		/* Extract one PATH element */
		while (*p && *p != ':' && dlen + 1 < sizeof(dir))
			dir[dlen++] = *p++;
		dir[dlen] = '\0';
		if (*p == ':')
			p++;

		if (dlen == 0)
			continue;

		DIR *d = opendir(dir);
		if (!d)
			continue;

		struct dirent *ent;
		size_t plen = strlen(clean);

		while ((ent = readdir(d)) != NULL && match_count < 128)
		{
			if (strncmp(ent->d_name, clean, plen) != 0)
				continue;

			/* Skip . and .. */
			if (plen > 0 &&
				(strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0))
				continue;

			char full[PATH_MAX];
			size_t nlen = strlen(ent->d_name);

			if (dlen + 1 + nlen >= sizeof(full))
				continue;

			memcpy(full, dir, dlen);
			full[dlen] = '/';
			memcpy(full + dlen + 1, ent->d_name, nlen);
			full[dlen + 1 + nlen] = '\0';

			struct stat st;
			if (stat(full, &st) != 0)
				continue;

			/* Require executable bit */
			if (!S_ISREG(st.st_mode))
				continue;
			if (access(full, X_OK) != 0)
				continue;

			/* Extension filtering */
			if (ce && ce->extensions)
			{
				bool ok = false;
				for (int k = 0; ce->extensions[k]; k++)
				{
					const char *ext = ce->extensions[k];
					size_t extlen = strlen(ext);

					if (nlen > extlen &&
						strcmp(ent->d_name + (nlen - extlen), ext) == 0)
					{
						ok = true;
						break;
					}
				}
				if (!ok)
					continue;
			}

			snprintf(matches[match_count], PATH_MAX, "%s", ent->d_name);
			matches[match_count][PATH_MAX - 1] = '\0';
			match_count++;
		}

		closedir(d);
	}

	LOG("autocomplete_cmd: match_count=%d", match_count);

	if (match_count == 0)
		return;

	qsort(matches, match_count, PATH_MAX, _cmp_match);

	if (match_count == 1)
	{
		const char *m = matches[0];
		LOG("autocomplete_cmd: single match='%s'", m);

		size_t plen = strlen(clean);
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

		LOG("autocomplete_cmd: completed buf='%s' len=%zu", buf, *len);
		return;
	}

	/* Multiple matches → print list */
	pager_columns(matches, match_count);
}

static void _input_autocomplete_path(char **argv, size_t *len, char *buf, autocomplete_type type)
{
	LOG("autocomplete_path: argv0='%s' argv1='%s' type=%d len=%zu buf='%s'",
		argv[0], argv[1], type, *len, buf);

	const char *prefix = (argv[1]) ? argv[1] : "";
	autocomplete_entry *ce = _input_autocomplete_lookup(argv[0]);
	LOG("autocomplete_path: prefix='%s'", prefix);

	/* Strip control chars */
	for (size_t i = 0; prefix[i]; i++)
		if ((unsigned char)prefix[i] < 32)
			((char *)prefix)[i] = '\0';

	LOG("autocomplete_path: sanitized prefix='%s'", prefix);

	/* Tilde expansion */
	if (prefix[0] == '~')
	{
		LOG("autocomplete_path: tilde detected");
		const char *home = getenv("HOME");
		LOG("autocomplete_path: HOME='%s'", home ? home : "(null)");

		if (!home)
			home = "/";

		static char expanded[PATH_MAX];

		if (prefix[1] == '/' || prefix[1] == '\0')
		{
			snprintf(expanded, sizeof(expanded), "%s%s", home, prefix + 1);
			prefix = expanded;
			LOG("autocomplete_path: expanded='%s'", prefix);
		}
	}

	/* ============================
	   Dynamic completion
	   ============================ */
	if (ce && type == AUTOCOMPLETE_DYNAMIC && ce->dynamic_prog)
	{
		LOG("autocomplete_path: dynamic helper='%s'", ce->dynamic_prog);

		FILE *pipe = popen(ce->dynamic_prog, "r");
		if (!pipe)
		{
			LOG("autocomplete_path: dynamic popen failed");
			return;
		}

		char matches[128][PATH_MAX];
		int match_count = 0;

		char line[PATH_MAX];
		while (fgets(line, sizeof(line), pipe) && match_count < 128)
		{
			char *nl = strchr(line, '\n');
			if (nl)
				*nl = '\0';

			if (strncmp(line, prefix, strlen(prefix)) == 0)
			{
				LOG("autocomplete_path: dynamic match '%s'", line);
				snprintf(matches[match_count], PATH_MAX, "%s", line);
				match_count++;
			}
			else
			{
				LOG("autocomplete_path: dynamic skip '%s'", line);
			}
		}

		pclose(pipe);

		LOG("autocomplete_path: dynamic total matches=%d", match_count);

		if (match_count == 0)
			return;

		qsort(matches, match_count, PATH_MAX, _cmp_match);

		if (match_count == 1)
		{
			const char *m = matches[0];
			LOG("autocomplete_path: dynamic single match '%s'", m);

			size_t plen = strlen(prefix);
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

			LOG("autocomplete_path: dynamic completed buf='%s' len=%zu", buf, *len);
			return;
		}

		LOG("autocomplete_path: dynamic multiple matches");
		pager_columns(matches, match_count);
		return;
	}

	char dirname[PATH_MAX] = {0};
	char partial[PATH_MAX] = {0};

	LOG("autocomplete_path: computing dirname/partial");

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

	LOG("autocomplete_path: dirname='%s' partial='%s'", dirname, partial);

	DIR *d = opendir(dirname);
	if (!d)
	{
		LOG("autocomplete_path: opendir failed");
		return;
	}

	LOG("autocomplete_path: scanning directory");

	struct dirent *ent;
	char matches[128][PATH_MAX];
	int match_count = 0;

	/* ============================================================
	   Scan directory entries
	   ============================================================ */
	while ((ent = readdir(d)) != NULL)
	{
		LOG("autocomplete_path: entry='%s'", ent->d_name);

		size_t plen = strlen(partial);
		if (strncmp(ent->d_name, partial, plen) != 0)
			continue;

		LOG("autocomplete_path: match='%s'", ent->d_name);

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

		if (type == AUTOCOMPLETE_EXECUTABLES)
		{
			if (!S_ISREG(st.st_mode))
			{
				LOG("autocomplete_path: skip '%s' (not regular)", ent->d_name);
				continue;
			}
			if (access(fullpath, X_OK) != 0)
			{
				LOG("autocomplete_path: skip '%s' (not executable)", ent->d_name);
				continue;
			}
		}

		if (ce && ce->extensions && S_ISREG(st.st_mode))
		{
			bool ok = false;

			for (int k = 0; ce->extensions[k]; k++)
			{
				const char *ext = ce->extensions[k];
				size_t extlen = strlen(ext);

				if (nlen > extlen &&
					strcmp(ent->d_name + (nlen - extlen), ext) == 0)
				{
					LOG("autocomplete_path: extension match '%s' ext='%s'", ent->d_name, ext);
					ok = true;
					break;
				}
			}

			if (!ok)
			{
				LOG("autocomplete_path: skip '%s' (extension mismatch)", ent->d_name);
				continue;
			}
		}

		snprintf(matches[match_count], PATH_MAX, "%s", ent->d_name);
		matches[match_count][PATH_MAX - 1] = '\0';

		match_count++;
		if (match_count >= 128)
			break;
	}
	closedir(d);

	LOG("autocomplete_path: match_count=%d", match_count);

	if (match_count == 0)
	{
		LOG("autocomplete_path: no matches");
		return;
	}

	LOG("autocomplete_path: sorting matches");
	qsort(matches, match_count, PATH_MAX, _cmp_match);

	/* ============================================================
	   Single match → complete it
	   ============================================================ */
	if (match_count == 1)
	{
		const char *m = matches[0];
		LOG("autocomplete_path: single match='%s'", m);

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

		LOG("autocomplete_path: completed buf='%s' len=%zu", buf, *len);
		return;
	}

	/* ============================================================
	   Multiple matches → print list
	   ============================================================ */
	LOG("autocomplete_path: multiple matches");
	pager_columns(matches, match_count);
}

/* ============================================================
 * PUBLIC API
 * ============================================================ */

void autocomplete_load_completions(void)
{
	LOG("autocomplete_load_completions: start");

	/* Load system-wide completions */
	_autocomplete_load_completions_dir("/usr/share/termlink/completions");
	_autocomplete_load_completions_dir("/etc/termlink/completions");

	/* Load user completions */
	const char *home = getenv("HOME");
	LOG("autocomplete_load_completions: HOME='%s'", home ? home : "(null)");

	if (home && *home)
	{
		char path[PATH_MAX];
		snprintf(path, sizeof(path), "%s/.termlink/completions", home);
		LOG("autocomplete_load_completions: user dir='%s'", path);
		_autocomplete_load_completions_dir(path);
	}

	/* Built-in fallback for cd */
	LOG("autocomplete_load_completions: adding built-in 'cd'");
	{
		autocomplete_entry *e;
		_autocomplete_register_completion("cd", AUTOCOMPLETE_DIRS);

		/* Fill optional fields with NULL */
		e = _input_autocomplete_lookup("cd");
		if (e)
		{
			e->options = NULL;
			e->extensions = NULL;
			e->dynamic_prog = NULL;
		}
	}

	LOG("autocomplete_load_completions: done");
}

void input_autocomplete_tab(input_state *st)
{
	LOG("autocomplete_tab: buf='%s' len=%zu cursor=%zu",
		st->buf, st->len, st->cursor);

	char temp[PATH_MAX];
	char *argv[MAX_ARGS];
	char *buf = st->buf;
	size_t *len = &st->len;

	/* Clear scroll region for match printing */
	LOG("autocomplete_tab: clearing scroll region");
	terminal_scroll_clear();
	terminal_cursor_line = terminal_scroll_top;

	/* Parse arguments from buffer */
	/* Copy only up to cursor for parsing */
	snprintf(temp, sizeof(temp), "%.*s", (int)st->cursor, st->buf);
	LOG("autocomplete_tab: temp='%s'", temp);

	int argc = input_parse_args(temp, argv, MAX_ARGS);
	LOG("autocomplete_tab: argc=%d", argc);

	if (argc == 0)
	{
		LOG("autocomplete_tab: no args, abort");
		return;
	}

	/* Save old buffer for change detection */
	char oldbuf[PATH_MAX];
	snprintf(oldbuf, sizeof(oldbuf), "%s", buf);
	LOG("autocomplete_tab: oldbuf='%s'", oldbuf);

	/* ===== Save suffix after cursor and limit completion to prefix ===== */
	char suffix[PATH_MAX];
	snprintf(suffix, sizeof(suffix), "%s", buf + st->cursor);
	LOG("autocomplete_tab: suffix='%s'", suffix);

	size_t original_len = *len;
	LOG("autocomplete_tab: original_len=%zu", original_len);

	*len = st->cursor;
	buf[*len] = '\0';
	LOG("autocomplete_tab: prefix='%s' new_len=%zu", buf, *len);

	/* ======================================================================= */

	/* If only command token → complete command name (external/builtin) */
	if (argc == 1)
	{
		/* If nothing typed, do nothing */
		if (argv[0][0] == '\0')
		{
			LOG("autocomplete_tab: empty command prefix, no completion");
			*len = original_len;
			buf[*len] = '\0';
			return;
		}

		LOG("autocomplete_tab: argc==1, command completion on argv[0]='%s'", argv[0]);
		_input_autocomplete_command(argv[0], len, buf);
	}
	else
	{
		/* Determine completion type from command */
		autocomplete_type type = AUTOCOMPLETE_FILES_AND_DIRS;
		autocomplete_entry *ce = _input_autocomplete_lookup(argv[0]);
		LOG("autocomplete_tab: lookup cmd='%s' result=%p", argv[0], ce);

		if (ce)
			type = ce->type;

		LOG("autocomplete_tab: type=%d", type);

		/* ============================================================
		   Determine which argument to complete
		   Rules:
			 - If only command: complete empty second arg
			 - If flags present: complete LAST non-flag argument
			 - Else: complete argv[1]
		   ============================================================ */

		int target_index = 1;

		if (argv[1][0] == '-' && argc > 2)
		{
			target_index = argc - 1;
			LOG("autocomplete_tab: flag detected, completing last arg index=%d", target_index);
		}
		else
		{
			LOG("autocomplete_tab: completing argv[1]");
			target_index = 1;
		}

		char *comp_argv[2];
		comp_argv[0] = argv[0];
		comp_argv[1] = argv[target_index];

		LOG("autocomplete_tab: comp_argv='%s' '%s'", comp_argv[0], comp_argv[1]);

		LOG("autocomplete_tab: calling input_autocomplete_path()");
		_input_autocomplete_path(comp_argv, len, buf, type);
	}

	LOG("autocomplete_tab: after completion buf='%s' len=%zu", buf, *len);

	/* ============================================================
	   Redraw ONLY if buffer changed
	   ============================================================ */
	if (strcmp(oldbuf, buf) != 0)
	{
		LOG("autocomplete_tab: buffer changed, reattaching suffix");

		/* ===== NEW: reattach suffix after completed prefix ===== */
		size_t prefix_len = *len;
		size_t suffix_len = strlen(suffix);

		LOG("autocomplete_tab: prefix_len=%zu suffix_len=%zu", prefix_len, suffix_len);

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
		LOG("autocomplete_tab: final buf='%s' len=%zu cursor=%zu",
			buf, *len, st->cursor);

		input_redraw(st, true);
	}
	else
	{
		LOG("autocomplete_tab: no change, restoring original len");
		/* restore len if no change */
		*len = original_len;
		buf[*len] = '\0';
	}
}