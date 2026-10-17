#include "main.h"
#include "input.h"
#include "terminal.h"

void input_redraw(input_state *st, bool place_cursor)
{
	terminal_draw_prompt();
	printf("\033[%d;%dH", terminal_rows, PROMPT_COL);
	printf("\033[K");
	printf("%s", st->buf);
	if (place_cursor)
		printf("\033[%d;%zuH", terminal_rows, PROMPT_COL + st->cursor);
	fflush(stdout);
}

int input_parse_args(char *line, char **argv, int max_args)
{
	int argc = 0;
	char *p = line;

	while (*p && argc < max_args - 1)
	{
		while (*p == ' ' || *p == '\t')
			p++;
		if (!*p)
			break;

		argv[argc++] = p;

		while (*p && *p != ' ' && *p != '\t')
			p++;
		if (*p)
		{
			*p = '\0';
			p++;
		}
	}

	argv[argc] = NULL;
	return argc;
}