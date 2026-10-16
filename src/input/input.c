#include "main.h"
#include "input.h"
#include "input_state.h"
#include "cursor.h"
#include "buffer.h"
#include "autocomplete.h"
#include "history.h"
#include "terminal.h"

static int input_read_char(void)
{
    int c = getchar();

    if (c != 27)
        return c;

    int c1 = getchar();
    if (c1 != '[') {
        ungetc(c1, stdin);
        return 27;
    }

    int c2 = getchar();

    if (c2 == 'A') return KEY_UP;
    if (c2 == 'B') return KEY_DOWN;
    if (c2 == 'C') return KEY_RIGHT;
    if (c2 == 'D') return KEY_LEFT;

    if (c2 == '3') {
        int c3 = getchar();
        if (c3 == '~') return KEY_DEL;
    }

    ungetc(c2, stdin);
    ungetc('[', stdin);
    return 27;
}

int input_parse_args(char *line, char **argv, int max_args) {
    int argc = 0;
    char *p = line;

    while (*p && argc < max_args - 1) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;

        argv[argc++] = p;

        while (*p && *p != ' ' && *p != '\t') p++;
        if (*p) {
            *p = '\0';
            p++;
        }
    }

    argv[argc] = NULL;
    return argc;
}

void input_read_line(char *buf, size_t maxlen)
{
    input_state st = {
        .buf = buf,
        .len = 0,
        .maxlen = maxlen,
        .cursor = 0,
        .insert_mode = 1,
        .tab_active = 0
    };

    terminal_draw_prompt();
    printf("\033[%d;%dH", terminal_rows, PROMPT_COL);

    while (1) {
        int ch = input_read_char();

        switch (ch) {

        case '\n':
        case '\r':
            st.buf[st.len] = '\0';
            terminal_draw_prompt();
            return;

        case 127:
        case '\b':
            input_buffer_backspace(&st);
            break;

        case '\t':
            input_autocomplete_tab(&st);
            printf("\033[%d;%dH", terminal_rows, PROMPT_COL);
            for (size_t i = 0; i < st.len; i++)
                terminal_slow_putc(st.buf[i]);
            printf("\033[%d;%dH", terminal_rows, PROMPT_COL + (int)st.len);
            fflush(stdout);
            break;

        case KEY_UP:
            history_prev(st.buf, st.maxlen);
            st.len = strlen(st.buf);
            st.cursor = st.len;

            terminal_draw_prompt();
            printf("\033[%d;%dH", terminal_rows, PROMPT_COL);
            printf("%s", st.buf);
            fflush(stdout);
            break;

        case KEY_DOWN:
            history_next(st.buf, st.maxlen);
            st.len = strlen(st.buf);
            st.cursor = st.len;

            terminal_draw_prompt();
            printf("\033[%d;%dH", terminal_rows, PROMPT_COL);
            printf("%s", st.buf);
            fflush(stdout);
            break;

        case KEY_LEFT:
            input_cursor_left(&st);
            break;

        case KEY_RIGHT:
            input_cursor_right(&st);
            break;

        case KEY_DEL:
            input_buffer_delete(&st);
            break;

        default:
            if (ch >= 32 && ch < 127) {
                input_buffer_insert(&st, (char)ch);
            }
            break;
        }
    }
}