#ifndef AUTOCOMPLETE_H
#define AUTOCOMPLETE_H

#include "main.h"
#include "input_state.h"

#define MAX_COMPLETIONS 128

typedef enum
{
	AUTOCOMPLETE_FILES,
	AUTOCOMPLETE_DIRS,
	AUTOCOMPLETE_FILES_AND_DIRS,
	AUTOCOMPLETE_EXECUTABLES,
	AUTOCOMPLETE_DYNAMIC
} autocomplete_type;

typedef struct
{
	const char *cmd;		  /* command name, e.g. "ls" */
	autocomplete_type type;	  /* default argument completion */
	const char **options;	  /* NULL‑terminated list of options, or NULL */
	const char **extensions;  /* NULL‑terminated list of file extensions, or NULL */
	const char *dynamic_prog; /* helper program for dynamic completion, or NULL */
} autocomplete_entry;

extern autocomplete_entry input_autocomplete_table[];
extern int input_autocomplete_count;

void input_autocomplete_path(char **argv, size_t *len, char *buf, autocomplete_type type);
void input_autocomplete_tab(input_state *st);
autocomplete_entry *input_autocomplete_lookup(const char *cmd);

#endif