#include "main.h"
#include "input_state.h"
#include "tokens.h"


static int _is_operator_char(char c)
{
	LOG("tokens: _is_operator_char('%c')", c);
	return (c == '|' || c == '&' || c == ';' || c == '>' || c == '<');
}

static int is_operator(const char *buf, int i)
{
	LOG("tokens: is_operator buf='%s' i=%d", buf, i);

	// Two-char operators
	if (i > 0)
	{
		if ((buf[i - 1] == '&' && buf[i] == '&') ||
			(buf[i - 1] == '|' && buf[i] == '|') ||
			(buf[i - 1] == '>' && buf[i] == '>') ||
			(buf[i - 1] == '<' && buf[i] == '<'))
		{
			LOG("tokens: two-char operator at %d", i);
			return 2;
		}
	}

	// Single-char operators
	if (_is_operator_char(buf[i]))
	{
		LOG("tokens: single-char operator at %d", i);
		return 1;
	}

	LOG("tokens: no operator at %d", i);
	return 0;
}

static int _scan_quote_left(const char *buf, int i)
{
	LOG("tokens: scan_quote_left buf='%s' i=%d", buf, i);

	char q = buf[i];
	i--;

	while (i >= 0)
	{
		if (buf[i] == q && buf[i - 1] != '\\')
		{
			LOG("tokens: quote-left found at %d", i);
			return i;
		}
		i--;
	}

	LOG("tokens: quote-left fallback to 0");
	return 0;
}

static int _scan_quote_right(const char *buf, int len, int i)
{
	LOG("tokens: scan_quote_right buf='%s' len=%d i=%d", buf, len, i);

	char q = buf[i];
	i++;

	while (i < len)
	{
		if (buf[i] == q && buf[i - 1] != '\\')
		{
			LOG("tokens: quote-right found at %d", i + 1);
			return i + 1;
		}
		i++;
	}

	LOG("tokens: quote-right fallback to len=%d", len);
	return len;
}

static int _scan_paren_left(const char *buf, int i)
{
	LOG("tokens: scan_paren_left buf='%s' i=%d", buf, i);

	int depth = 1;
	i--;

	while (i >= 0)
	{
		if (buf[i] == ')')
			depth++;
		else if (buf[i] == '(')
			depth--;

		if (depth == 0)
		{
			LOG("tokens: paren-left found at %d", i);
			return i;
		}

		i--;
	}

	LOG("tokens: paren-left fallback to 0");
	return 0;
}

static int _scan_paren_right(const char *buf, int len, int i)
{
	LOG("tokens: scan_paren_right buf='%s' len=%d i=%d", buf, len, i);

	int depth = 1;
	i++;

	while (i < len)
	{
		if (buf[i] == '(')
			depth++;
		else if (buf[i] == ')')
			depth--;

		if (depth == 0)
		{
			LOG("tokens: paren-right found at %d", i + 1);
			return i + 1;
		}

		i++;
	}

	LOG("tokens: paren-right fallback to len=%d", len);
	return len;
}

int input_tokens_prev(const char *buf, int cursor)
{
	LOG("tokens_prev: buf='%s' cursor=%d", buf, cursor);

	if (cursor <= 0)
	{
		LOG("tokens_prev: cursor<=0 → return 0");
		return 0;
	}

	int i = cursor - 1;

	// Skip whitespace
	while (i > 0 && isspace((unsigned char)buf[i]))
		i--;

	LOG("tokens_prev: after skip whitespace i=%d", i);

	// Quoted string?
	if (buf[i] == '"' || buf[i] == '\'')
	{
		int r = _scan_quote_left(buf, i);
		LOG("tokens_prev: quote-left result=%d", r);
		return r;
	}

	// Subshell?
	if (buf[i] == ')')
	{
		int r = _scan_paren_left(buf, i);
		LOG("tokens_prev: paren-left result=%d", r);
		return r;
	}

	// Operator?
	int op = is_operator(buf, i);
	if (op)
	{
		LOG("tokens_prev: operator size=%d", op);
		return i - (op - 1);
	}

	// Normal token
	while (i > 0 && !isspace((unsigned char)buf[i]) && !_is_operator_char(buf[i]))
		i--;

	if (isspace((unsigned char)buf[i]) || _is_operator_char(buf[i]))
	{
		LOG("tokens_prev: normal token start=%d", i + 1);
		return i + 1;
	}

	LOG("tokens_prev: fallback=%d", i);
	return i;
}

int input_tokens_next(const char *buf, int cursor)
{
	LOG("tokens_next: buf='%s' cursor=%d", buf, cursor);

	int len = strlen(buf);
	if (cursor >= len)
	{
		LOG("tokens_next: cursor>=len → return len=%d", len);
		return len;
	}

	int i = cursor;

	// Skip whitespace
	while (i < len && isspace((unsigned char)buf[i]))
		i++;

	LOG("tokens_next: after skip whitespace i=%d", i);

	if (i >= len)
	{
		LOG("tokens_next: i>=len → return len=%d", len);
		return len;
	}

	// Quoted string?
	if (buf[i] == '"' || buf[i] == '\'')
	{
		int r = _scan_quote_right(buf, len, i);
		LOG("tokens_next: quote-right result=%d", r);
		return r;
	}

	// Subshell?
	if (buf[i] == '(')
	{
		int r = _scan_paren_right(buf, len, i);
		LOG("tokens_next: paren-right result=%d", r);
		return r;
	}

	// Operator?
	int op = is_operator(buf, i);
	if (op)
	{
		LOG("tokens_next: operator size=%d", op);
		return i + op;
	}

	// Normal token
	while (i < len && !isspace((unsigned char)buf[i]) && !_is_operator_char(buf[i]))
		i++;

	LOG("tokens_next: normal token end=%d", i);
	return i;
}

void input_tokens_delete_prev(input_state *st)
{
	LOG("tokens_delete_prev: buf='%s' cursor=%zu len=%zu",
		st->buf, st->cursor, st->len);

	int start = input_tokens_prev(st->buf, st->cursor);
	int end = st->cursor;

	LOG("tokens_delete_prev: start=%d end=%d", start, end);

	if (start == end)
	{
		LOG("tokens_delete_prev: nothing to delete");
		return;
	}

	// Shift buffer left
	memmove(st->buf + start, st->buf + end, st->len - end + 1);

	st->len -= (end - start);
	st->cursor = start;

	LOG("tokens_delete_prev: after delete buf='%s' cursor=%zu len=%zu",
		st->buf, st->cursor, st->len);
}

void input_tokens_delete_next(input_state *st)
{
	LOG("tokens_delete_next: buf='%s' cursor=%zu len=%zu",
		st->buf, st->cursor, st->len);

	int start = st->cursor;
	int end = input_tokens_next(st->buf, st->cursor);

	LOG("tokens_delete_next: start=%d end=%d", start, end);

	if (start == end)
	{
		LOG("tokens_delete_next: nothing to delete");
		return;
	}

	memmove(st->buf + start, st->buf + end, st->len - end + 1);

	st->len -= (end - start);

	LOG("tokens_delete_next: after delete buf='%s' cursor=%zu len=%zu",
		st->buf, st->cursor, st->len);
}

void input_tokens_kill_to_begin(input_state *st)
{
	LOG("tokens_kill_to_begin: buf='%s' cursor=%zu len=%zu",
		st->buf, st->cursor, st->len);

	// Save killed text
	st->kill_len = st->cursor;
	memcpy(st->killbuf, st->buf, st->kill_len);
	st->killbuf[st->kill_len] = '\0';

	LOG("tokens_kill_to_begin: saved killbuf='%s' kill_len=%zu",
		st->killbuf, st->kill_len);

	// Shift buffer left
	memmove(st->buf, st->buf + st->cursor, st->len - st->cursor + 1);

	st->len -= st->cursor;
	st->cursor = 0;

	LOG("tokens_kill_to_begin: after kill buf='%s' len=%zu cursor=%zu",
		st->buf, st->len, st->cursor);
}

void input_tokens_kill_to_end(input_state *st)
{
	LOG("tokens_kill_to_end: buf='%s' cursor=%zu len=%zu",
		st->buf, st->cursor, st->len);

	// Save killed text
	st->kill_len = st->len - st->cursor;
	memcpy(st->killbuf, st->buf + st->cursor, st->kill_len);
	st->killbuf[st->kill_len] = '\0';

	LOG("tokens_kill_to_end: saved killbuf='%s' kill_len=%zu",
		st->killbuf, st->kill_len);

	// Truncate buffer
	st->buf[st->cursor] = '\0';
	st->len = st->cursor;

	LOG("tokens_kill_to_end: after kill buf='%s' len=%zu cursor=%zu",
		st->buf, st->len, st->cursor);
}

void input_tokens_yank(input_state *st)
{
	LOG("tokens_yank: buf='%s' cursor=%zu len=%zu killbuf='%s' kill_len=%zu",
		st->buf, st->cursor, st->len, st->killbuf, st->kill_len);

	if (st->kill_len == 0)
	{
		LOG("tokens_yank: kill_len==0, abort");
		return;
	}

	// Ensure space
	if (st->len + st->kill_len >= st->maxlen)
	{
		LOG("tokens_yank: no space, abort");
		return;
	}

	// Shift right
	memmove(st->buf + st->cursor + st->kill_len,
			st->buf + st->cursor,
			st->len - st->cursor + 1);

	// Insert kill buffer
	memcpy(st->buf + st->cursor, st->killbuf, st->kill_len);

	st->len += st->kill_len;
	st->cursor += st->kill_len;

	LOG("tokens_yank: after yank buf='%s' cursor=%zu len=%zu",
		st->buf, st->cursor, st->len);
}