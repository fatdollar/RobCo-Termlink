#include "main.h"
#include "tl.h"

/* ============================================================
 * FORWARD DECLARATIONS
 * ============================================================ */

static bool _tl_glob_needs_expansion(const char *s);
static void _tl_glob_free_word_list(tl_word_t *w);
static tl_word_t *_tl_glob_new_word(const char *text);
static void _tl_glob_append_word(tl_word_t **head, tl_word_t **tail, tl_word_t *w);
static void _tl_glob_collect(const char *pattern, tl_word_t **head, tl_word_t **tail);
static bool _tl_glob_match(const char *pat, const char *str);

/* ============================================================
 * INTERNAL HELPERS
 * ============================================================ */

static bool _tl_glob_needs_expansion(const char *s)
{
	return strpbrk(s, "*?[") != NULL;
}

static void _tl_glob_free_word_list(tl_word_t *w)
{
	while (w)
	{
		tl_word_t *next = w->next;
		free(w->text);
		free(w);
		w = next;
	}
}

static tl_word_t *_tl_glob_new_word(const char *text)
{
	tl_word_t *w = calloc(1, sizeof(tl_word_t));
	w->text = strdup(text);
	return w;
}

static void _tl_glob_append_word(tl_word_t **head, tl_word_t **tail, tl_word_t *w)
{
	if (!*head)
		*head = *tail = w;
	else
	{
		(*tail)->next = w;
		*tail = w;
	}
}

static void _tl_glob_collect(const char *pattern, tl_word_t **head, tl_word_t **tail)
{
	DIR *d = opendir(".");
	if (!d)
		return;

	struct dirent *ent;

	while ((ent = readdir(d)) != NULL)
	{
		if (_tl_glob_match(pattern, ent->d_name))
		{
			tl_word_t *w = calloc(1, sizeof(tl_word_t));
			w->text = strdup(ent->d_name);
			_tl_glob_append_word(head, tail, w);
		}
	}

	closedir(d);
}

static bool _tl_glob_match(const char *pat, const char *str)
{
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

tl_word_t *tl_expand_globs_on_argv(tl_word_t *argv)
{
	tl_word_t *new_head = NULL;
	tl_word_t *new_tail = NULL;

	for (tl_word_t *w = argv; w; w = w->next)
	{
		const char *arg = w->text;

		if (!_tl_glob_needs_expansion(arg))
		{
			/* literal → copy */
			tl_word_t *copy = _tl_glob_new_word(arg);
			_tl_glob_append_word(&new_head, &new_tail, copy);
			continue;
		}

		tl_word_t *matches_head = NULL;
		tl_word_t *matches_tail = NULL;

		_tl_glob_collect(arg, &matches_head, &matches_tail);

		if (matches_head)
		{
			/* matched → append all */
			for (tl_word_t *m = matches_head; m; m = m->next)
			{
				tl_word_t *copy = _tl_glob_new_word(m->text);
				_tl_glob_append_word(&new_head, &new_tail, copy);
			}

			_tl_glob_free_word_list(matches_head);
		}
		else
		{
			/* no matches → literal */
			tl_word_t *copy = _tl_glob_new_word(arg);
			_tl_glob_append_word(&new_head, &new_tail, copy);
		}
	}

	/* free old argv list */
	_tl_glob_free_word_list(argv);

	return new_head;
}