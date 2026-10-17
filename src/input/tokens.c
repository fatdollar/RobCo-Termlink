#include "main.h"
#include "input_state.h"
#include "tokens.h"

static int _is_operator_char(char c)
{
    return (c == '|' || c == '&' || c == ';' || c == '>' || c == '<');
}

static int is_operator(const char *buf, int i)
{
    // Two-char operators
    if (i > 0)
    {
        if ((buf[i - 1] == '&' && buf[i] == '&') ||
            (buf[i - 1] == '|' && buf[i] == '|') ||
            (buf[i - 1] == '>' && buf[i] == '>') ||
            (buf[i - 1] == '<' && buf[i] == '<'))
            return 2;
    }

    // Single-char operators
    if (_is_operator_char(buf[i]))
        return 1;

    return 0;
}

static int _scan_quote_left(const char *buf, int i)
{
    char q = buf[i];
    i--;

    while (i >= 0)
    {
        if (buf[i] == q && buf[i - 1] != '\\')
            return i;
        i--;
    }

    return 0;
}

static int _scan_quote_right(const char *buf, int len, int i)
{
    char q = buf[i];
    i++;

    while (i < len)
    {
        if (buf[i] == q && buf[i - 1] != '\\')
            return i + 1;
        i++;
    }

    return len;
}

static int _scan_paren_left(const char *buf, int i)
{
    int depth = 1;
    i--;

    while (i >= 0)
    {
        if (buf[i] == ')')
            depth++;
        else if (buf[i] == '(')
            depth--;

        if (depth == 0)
            return i;

        i--;
    }

    return 0;
}

static int _scan_paren_right(const char *buf, int len, int i)
{
    int depth = 1;
    i++;

    while (i < len)
    {
        if (buf[i] == '(')
            depth++;
        else if (buf[i] == ')')
            depth--;

        if (depth == 0)
            return i + 1;

        i++;
    }

    return len;
}

int input_tokens_prev(const char *buf, int cursor)
{
    if (cursor <= 0)
        return 0;

    int i = cursor - 1;

    // Skip whitespace
    while (i > 0 && isspace((unsigned char)buf[i]))
        i--;

    // Quoted string?
    if (buf[i] == '"' || buf[i] == '\'')
        return _scan_quote_left(buf, i);

    // Subshell?
    if (buf[i] == ')')
        return _scan_paren_left(buf, i);

    // Operator?
    int op = is_operator(buf, i);
    if (op)
        return i - (op - 1);

    // Normal token
    while (i > 0 && !isspace((unsigned char)buf[i]) && !_is_operator_char(buf[i]))
        i--;

    if (isspace((unsigned char)buf[i]) || _is_operator_char(buf[i]))
        return i + 1;

    return i;
}

int input_tokens_next(const char *buf, int cursor)
{
    int len = strlen(buf);
    if (cursor >= len)
        return len;

    int i = cursor;

    // Skip whitespace
    while (i < len && isspace((unsigned char)buf[i]))
        i++;

    if (i >= len)
        return len;

    // Quoted string?
    if (buf[i] == '"' || buf[i] == '\'')
        return _scan_quote_right(buf, len, i);

    // Subshell?
    if (buf[i] == '(')
        return _scan_paren_right(buf, len, i);

    // Operator?
    int op = is_operator(buf, i);
    if (op)
        return i + op;

    // Normal token
    while (i < len && !isspace((unsigned char)buf[i]) && !_is_operator_char(buf[i]))
        i++;

    return i;
}

void input_tokens_delete_prev(input_state *st)
{
    int start = input_tokens_prev(st->buf, st->cursor);
    int end = st->cursor;

    if (start == end)
        return;

    // Shift buffer left
    memmove(st->buf + start, st->buf + end, st->len - end + 1);

    st->len -= (end - start);
    st->cursor = start;
}

void input_tokens_delete_next(input_state *st)
{
    int start = st->cursor;
    int end = input_tokens_next(st->buf, st->cursor);

    if (start == end)
        return;

    memmove(st->buf + start, st->buf + end, st->len - end + 1);

    st->len -= (end - start);
}

void input_tokens_kill_to_begin(input_state *st)
{
    // Save killed text
    st->kill_len = st->cursor;
    memcpy(st->killbuf, st->buf, st->kill_len);
    st->killbuf[st->kill_len] = '\0';

    // Shift buffer left
    memmove(st->buf, st->buf + st->cursor, st->len - st->cursor + 1);

    st->len -= st->cursor;
    st->cursor = 0;
}

void input_tokens_kill_to_end(input_state *st)
{
    // Save killed text
    st->kill_len = st->len - st->cursor;
    memcpy(st->killbuf, st->buf + st->cursor, st->kill_len);
    st->killbuf[st->kill_len] = '\0';

    // Truncate buffer
    st->buf[st->cursor] = '\0';
    st->len = st->cursor;
}

void input_tokens_yank(input_state *st)
{
    if (st->kill_len == 0)
        return;

    // Ensure space
    if (st->len + st->kill_len >= st->maxlen)
        return;

    // Shift right
    memmove(st->buf + st->cursor + st->kill_len,
            st->buf + st->cursor,
            st->len - st->cursor + 1);

    // Insert kill buffer
    memcpy(st->buf + st->cursor, st->killbuf, st->kill_len);

    st->len += st->kill_len;
    st->cursor += st->kill_len;
}