#ifndef INPUT_STATE_H
#define INPUT_STATE_H

#include "history.h"

typedef struct
{
	char *buf;	   // pointer to input buffer
	size_t len;	   // current length of buffer
	size_t maxlen; // maximum buffer size

	size_t cursor; // cursor position (0..len)

	int insert_mode; // always 1 for now, but allows replace-mode later
	int tab_active;	 // for multi-step completions (Phase 5)

	history_entry hist; // history tracking struct

	char killbuf[1024]; // kill buffer to restor if needed
	size_t kill_len; // length of kill buffer
} input_state;

#endif