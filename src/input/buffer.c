#include "main.h"
#include "buffer.h"
#include "terminal.h"

void input_buffer_insert(input_state *st, char c)
{
	LOG("buffer_insert: c='%c' cursor=%zu len=%zu maxlen=%zu buf='%s'",
		c, st->cursor, st->len, st->maxlen, st->buf);

	if (st->len >= st->maxlen - 1)
		return;

	memmove(st->buf + st->cursor + 1,
			st->buf + st->cursor,
			st->len - st->cursor);

	st->buf[st->cursor] = c;
	st->cursor++;
	st->len++;

	st->buf[st->len] = '\0';

	LOG("buffer_insert: after insert buf='%s' cursor=%zu len=%zu",
		st->buf, st->cursor, st->len);
}

void input_buffer_backspace(input_state *st)
{
	LOG("buffer_backspace: cursor=%zu len=%zu buf='%s'",
		st->cursor, st->len, st->buf);

	if (st->cursor == 0)
		return;

	memmove(st->buf + st->cursor - 1,
			st->buf + st->cursor,
			st->len - st->cursor);

	st->cursor--;
	st->len--;

	st->buf[st->len] = '\0';

	LOG("buffer_backspace: after delete buf='%s' cursor=%zu len=%zu",
		st->buf, st->cursor, st->len);
}

void input_buffer_delete(input_state *st)
{
	LOG("buffer_delete: cursor=%zu len=%zu buf='%s'",
		st->cursor, st->len, st->buf);

	if (st->cursor >= st->len)
		return;

	memmove(st->buf + st->cursor,
			st->buf + st->cursor + 1,
			st->len - st->cursor - 1);

	st->len--;
	
	st->buf[st->len] = '\0';

	LOG("buffer_delete: after delete buf='%s' cursor=%zu len=%zu",
		st->buf, st->cursor, st->len);
}