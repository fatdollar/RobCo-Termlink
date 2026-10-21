#include "main.h"
#include "tl.h"

/* ============================================================
 * FORWARD DECLARATIONS
 * ============================================================ */

static bool _tl_glob_needs_expansion(const char *s);
static void _tl_glob_free_word_list(tl_word_t *w);
static tl_word_t *_tl_glob_new_word(const char *text);
static void _tl_glob_append_word(tl_word_t **head, tl_word_t **tail, tl_word_t *w);
static void _tl_glob_split_pattern(const char *pattern, char *dir, size_t dir_size, char *filepat, size_t filepat_size);
static void _tl_glob_collect(const char *pattern, tl_word_t **head, tl_word_t **tail);
static bool _tl_glob_match(const char *pat, const char *str);

/* ============================================================
 * INTERNAL HELPERS
 * ============================================================ */

static bool _tl_glob_needs_expansion(const char *s)
{
	bool expand = strpbrk(s, "*?[") != NULL;

	LOG("_tl_glob_needs_expansion: '%s' -> %s", s, expand ? "true" : "false");

	return expand;
}

static void _tl_glob_free_word_list(tl_word_t *w)
{
	LOG("_tl_glob_free_word_list: start");

	while (w)
	{
		tl_word_t *next = w->next;

		LOG("_tl_glob_free_word_list: freeing '%s'",
			w->text);

		free(w->text);
		free(w);

		w = next;
	}

	LOG("_tl_glob_free_word_list: done");
}

static tl_word_t *_tl_glob_new_word(const char *text)
{
	LOG("_tl_glob_new_word: '%s'", text);

	tl_word_t *w = calloc(1, sizeof(tl_word_t));
	w->text = strdup(text);

	return w;
}

static void _tl_glob_append_word(tl_word_t **head, tl_word_t **tail, tl_word_t *w)
{
	LOG("_tl_glob_append_word: '%s'", w->text);

	if (!*head)
		*head = *tail = w;
	else
	{
		(*tail)->next = w;
		*tail = w;
	}
}

static void _tl_glob_split_pattern(const char *pattern, char *dir, size_t dir_size, char *filepat, size_t filepat_size)
{
	const char *slash = strrchr(pattern, '/');

	if (!slash)
	{
		strncpy(dir, ".", dir_size);
		dir[dir_size - 1] = '\0';

		strncpy(filepat, pattern, filepat_size);
		filepat[filepat_size - 1] = '\0';

		LOG("_tl_glob_split_pattern: '%s' -> dir='.' filepat='%s'",
			pattern,
			filepat);
		return;
	}

	size_t dir_len = (size_t)(slash - pattern);

	if (dir_len >= dir_size)
		dir_len = dir_size - 1;

	memcpy(dir, pattern, dir_len);
	dir[dir_len] = '\0';

	strncpy(filepat, slash + 1, filepat_size);
	filepat[filepat_size - 1] = '\0';

	LOG("_tl_glob_split_pattern: '%s' -> dir='%s' filepat='%s'",
		pattern,
		dir,
		filepat);
}

static void _tl_glob_collect(const char *pattern, tl_word_t **head, tl_word_t **tail)
{
	LOG("_tl_glob_collect: pattern='%s'", pattern);

	char dir[PATH_MAX];
	char filepat[PATH_MAX];

	_tl_glob_split_pattern(pattern, dir, sizeof(dir), filepat, sizeof(filepat));
	LOG("_tl_glob_collect: dir='%s' filepat='%s'", dir, filepat);

	DIR *d = opendir(dir);
	if (!d)
	{
		LOG("_tl_glob_collect: opendir failed");
		return;
	}

	struct dirent *ent;

	while ((ent = readdir(d)) != NULL)
	{
		LOG("_tl_glob_collect: checking '%s'",
			ent->d_name);

		if (_tl_glob_match(filepat, ent->d_name))
		{
			LOG("_tl_glob_collect: MATCH '%s'", ent->d_name);

			tl_word_t *w = calloc(1, sizeof(tl_word_t));

			if (strcmp(dir, ".") == 0)
			{
				w->text = strdup(ent->d_name);
			}
			else
			{
				size_t path_len = strlen(dir) + strlen(ent->d_name) + 2;

				char *fullpath = malloc(path_len);
				if (!fullpath)
				{
					LOG("_tl_glob_collect: malloc failed");
					continue;
				}

				snprintf(fullpath, path_len, "%s/%s", dir, ent->d_name);

				LOG("_tl_glob_collect: reconstructed='%s'", fullpath);

				w->text = fullpath;
			}

			_tl_glob_append_word(head, tail, w);
		}
	}

	closedir(d);

	LOG("_tl_glob_collect: done");
}

static bool _tl_glob_match(const char *pat, const char *str)
{
	LOG("_tl_glob_match: pat='%s' str='%s'", pat, str);

	while (*pat)
	{
		if (*pat == '*')
		{
			pat++;
			if (!*pat)
				return true; /* trailing * matches everything */

			while (*str)
			{
				if (_tl_glob_match(pat, str))
					return true;
				str++;
			}
			return false;
		}
		else if (*pat == '?')
		{
			if (!*str)
				return false;
			pat++;
			str++;
		}
		else if (*pat == '[')
		{
			pat++;
			bool ok = false;

			while (*pat && *pat != ']')
			{
				if (*pat == *str)
					ok = true;
				pat++;
			}

			if (*pat != ']')
				return false; /* malformed */

			if (!ok)
				return false;

			pat++;
			str++;
		}
		else
		{
			if (*pat != *str)
				return false;
			pat++;
			str++;
		}
	}

	return *str == '\0';
}

/* ---------------------------------------------------------
 *  GLOB EXPANSION FOR ARGV: tl_word_t list → new tl_word_t list
 * --------------------------------------------------------- */

tl_word_t *tl_glob_expand_on_argv(tl_word_t *argv)
{
	LOG("tl_glob_expand_on_argv: start");

	tl_word_t *new_head = NULL;
	tl_word_t *new_tail = NULL;

	for (tl_word_t *w = argv; w; w = w->next)
	{
		const char *arg = w->text;

		LOG("tl_glob_expand_on_argv: arg='%s'", arg);

		if (!_tl_glob_needs_expansion(arg))
		{
			LOG("tl_glob_expand_on_argv: literal");

			tl_word_t *copy = _tl_glob_new_word(arg);
			_tl_glob_append_word(
				&new_head,
				&new_tail,
				copy);

			continue;
		}

		LOG("tl_glob_expand_on_argv: expanding '%s'",
			arg);

		tl_word_t *matches_head = NULL;
		tl_word_t *matches_tail = NULL;

		_tl_glob_collect(
			arg,
			&matches_head,
			&matches_tail);

		if (matches_head)
		{
			LOG("tl_glob_expand_on_argv: matches found");

			for (tl_word_t *m = matches_head;
				 m;
				 m = m->next)
			{
				LOG("tl_glob_expand_on_argv: match='%s'",
					m->text);

				tl_word_t *copy =
					_tl_glob_new_word(m->text);

				_tl_glob_append_word(
					&new_head,
					&new_tail,
					copy);
			}

			_tl_glob_free_word_list(matches_head);
		}
		else
		{
			LOG("tl_glob_expand_on_argv: no matches");

			tl_word_t *copy =
				_tl_glob_new_word(arg);

			_tl_glob_append_word(
				&new_head,
				&new_tail,
				copy);
		}
	}

	_tl_glob_free_word_list(argv);

	LOG("tl_glob_expand_on_argv: done");

	return new_head;
}