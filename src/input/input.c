#include "main.h"
#include "input.h"
#include "terminal.h"

void input_redraw(input_state *st, bool place_cursor)
{
	LOG("input_redraw: buf='%s' len=%zu cursor=%zu place_cursor=%d",
		st->buf, st->len, st->cursor, place_cursor);

	// Move to buffer start (right of prompt)
	printf("\033[%d;%dH", terminal_rows, PROMPT_COL);

	// Clear only buffer area
	printf("\033[0K");

	// Print buffer
	for (size_t i = 0; i < st->len; i++)
		putchar(st->buf[i]);

	if (place_cursor)
		printf("\033[%d;%zuH", terminal_rows, PROMPT_COL + st->cursor);

	fflush(stdout);
}

int input_parse_args(char *line, char **argv, int max)
{
	LOG("input_parse_args: start");

	int argc = 0;
	char *p = line;

	while (*p && argc < max)
	{
		/* Skip whitespace */
		while (*p && isspace((unsigned char)*p))
			p++;

		if (!*p)
			break;

		char token[PATH_MAX];
		int ti = 0;

		/* Normal unquoted token */
		if (*p != '"' && *p != '\'')
		{
			while (*p && !isspace((unsigned char)*p))
			{
				if (*p == '\\' && p[1])
				{
					/* Escape next char */
					token[ti++] = p[1];
					p += 2;
					continue;
				}

				if (*p == '"' || *p == '\'')
					break;

				token[ti++] = *p++;
			}
		}
		else if (*p == '"')
		{
			/* Double-quoted token */
			p++; /* skip " */
			while (*p && *p != '"')
			{
				if (*p == '\\' && p[1])
				{
					token[ti++] = p[1];
					p += 2;
					continue;
				}
				token[ti++] = *p++;
			}
			if (*p == '"')
				p++; /* skip closing " */
		}
		else if (*p == '\'')
		{
			/* Single-quoted token */
			p++; /* skip ' */
			while (*p && *p != '\'')
			{
				/* Single quotes: literal, no escapes */
				token[ti++] = *p++;
			}
			if (*p == '\'')
				p++; /* skip closing ' */
		}

		token[ti] = '\0';

		LOG("input_parse_args: token='%s'", token);

		argv[argc] = strdup(token);
		argc++;
	}

	LOG("input_parse_args: done, argc=%d", argc);
	return argc;
}