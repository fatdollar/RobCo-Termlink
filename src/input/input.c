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

int input_parse_args(char *line, char **argv, int max_args)
{
	LOG("parse_args: line='%s' max_args=%d", line, max_args);

	int argc = 0;
	char *p = line;

	while (*p && argc < max_args - 1)
	{
		while (*p == ' ' || *p == '\t')
			p++;
		if (!*p)
			break;

		argv[argc++] = p;
		LOG("parse_args: arg[%d]='%s'", argc - 1, p);

		while (*p && *p != ' ' && *p != '\t')
			p++;
		if (*p)
		{
			*p = '\0';
			p++;
		}
	}

	argv[argc] = NULL;

	LOG("parse_args: argc=%d", argc);
	return argc;
}