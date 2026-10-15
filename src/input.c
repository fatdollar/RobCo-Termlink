#include "main.h"
#include "input.h"
#include "terminal.h"
#include "util.h"
#include "history.h"

completion_entry input_completion_table[MAX_COMPLETIONS];
int input_completion_count = 0;

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

completion_entry *input_lookup_completion(const char *cmd) {
    for (int i = 0; i < input_completion_count; i++) {
        if (strcmp(input_completion_table[i].cmd, cmd) == 0)
            return &input_completion_table[i];
    }
    return NULL;
}

static int cmp_match(const void *a, const void *b) {
    const char *sa = (const char *)a;
    const char *sb = (const char *)b;
    return strcmp(sa, sb);
}
void input_complete_path(char **argv, size_t *len, char *buf, completion_type type)
{
    const char *prefix = (argv[1]) ? argv[1] : "";

	for (size_t i = 0; prefix[i]; i++)
		if ((unsigned char)prefix[i] < 32)
			((char *)prefix)[i] = '\0';

    /* Tilde expansion */
    if (prefix[0] == '~') {
        const char *home = getenv("HOME");
        if (!home)
            home = "/";

        static char expanded[PATH_MAX];

        if (prefix[1] == '/' || prefix[1] == '\0') {
            snprintf(expanded, sizeof(expanded), "%s%s", home, prefix + 1);
            prefix = expanded;
        }
    }

	char dirname[PATH_MAX] = {0};
	char partial[PATH_MAX] = {0};

    /* ============================================================
       Compute dirname + partial from prefix
       ============================================================ */
    if (prefix[0] == '/')
    {
        char *slash = strrchr(prefix, '/');

        if (slash == prefix)
        {
            snprintf(dirname, sizeof(dirname), "/");
            snprintf(partial, sizeof(partial), "%s", prefix + 1);
        }
        else
        {
            size_t dlen = (size_t)(slash - prefix);
            if (dlen >= sizeof(dirname))
                dlen = sizeof(dirname) - 1;

            memcpy(dirname, prefix, dlen);
            dirname[dlen] = '\0';

            snprintf(partial, sizeof(partial), "%s", slash + 1);
        }
    }
    else
    {
        char *slash = strrchr(prefix, '/');
        if (slash)
        {
            size_t dlen = (size_t)(slash - prefix);
            if (dlen >= sizeof(dirname))
                dlen = sizeof(dirname) - 1;

            memcpy(dirname, prefix, dlen);
            dirname[dlen] = '\0';

            snprintf(partial, sizeof(partial), "%s", slash + 1);
        }
        else
        {
            if (!getcwd(dirname, sizeof(dirname)))
                snprintf(dirname, sizeof(dirname), ".");

            snprintf(partial, sizeof(partial), "%s", prefix);
        }
    }

    DIR *d = opendir(dirname);
    if (!d)
        return;

    struct dirent *ent;
    char matches[128][PATH_MAX];
    int match_count = 0;

    /* ============================================================
       Scan directory entries
       ============================================================ */
    while ((ent = readdir(d)) != NULL)
    {
        size_t plen = strlen(partial);
        if (strncmp(ent->d_name, partial, plen) != 0)
            continue;

        if (plen > 0 &&
            (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0))
            continue;

        char fullpath[PATH_MAX];
        size_t dlen = strlen(dirname);
        size_t nlen = strlen(ent->d_name);

        if (dlen + 1 + nlen >= PATH_MAX)
            continue;

        memcpy(fullpath, dirname, dlen);
        fullpath[dlen] = '/';
        memcpy(fullpath + dlen + 1, ent->d_name, nlen);
        fullpath[dlen + 1 + nlen] = '\0';

        struct stat st;
        if (stat(fullpath, &st) != 0)
            continue;

        if (type == COMPLETE_DIRS && !S_ISDIR(st.st_mode))
            continue;
        if (type == COMPLETE_FILES && !S_ISREG(st.st_mode))
            continue;

        snprintf(matches[match_count], PATH_MAX, "%s", ent->d_name);
        matches[match_count][PATH_MAX - 1] = '\0';

        match_count++;
        if (match_count >= 128)
            break;
    }
    closedir(d);

    if (match_count == 0)
        return;

    /* Sort matches */
    qsort(matches, match_count, PATH_MAX, cmp_match);

    /* ============================================================
       Single match → complete it
       ============================================================ */
    if (match_count == 1)
    {
        const char *m = matches[0];

        size_t plen = strlen(partial);
        size_t mlen = strlen(m);

        if (*len >= plen)
            *len -= plen;
        buf[*len] = '\0';

        if (*len + mlen < PATH_MAX)
        {
            memcpy(buf + *len, m, mlen);
            *len += mlen;
            buf[*len] = '\0';
        }

        char fullpath[PATH_MAX];
        size_t dlen = strlen(dirname);
        size_t nlen = strlen(m);

        if (dlen + 1 + nlen < PATH_MAX)
        {
            memcpy(fullpath, dirname, dlen);
            fullpath[dlen] = '/';
            memcpy(fullpath + dlen + 1, m, nlen);
            fullpath[dlen + 1 + nlen] = '\0';

            struct stat st;
            if (stat(fullpath, &st) == 0 && S_ISDIR(st.st_mode))
            {
                if (*len + 1 < PATH_MAX)
                {
                    buf[*len] = '/';
                    (*len)++;
                    buf[*len] = '\0';
                }
            }
        }

        return;
    }

    /* ============================================================
       Multiple matches → print list
       ============================================================ */
    terminal_slow_print("Matches:");
    for (int i = 0; i < match_count; i++)
        terminal_slow_print(matches[i]);
}

void input_handle_tab(char *buf, size_t *len)
{
    char temp[PATH_MAX];
    char *argv[MAX_ARGS];

    /* Clear scroll region for match printing */
    terminal_clear_scroll();
    terminal_cursor_line = terminal_scroll_top;

    /* Parse arguments from buffer */
    snprintf(temp, sizeof(temp), "%s", buf);
    int argc = input_parse_args(temp, argv, MAX_ARGS);
    if (argc == 0)
        return;

    /* Determine completion type from command */
    completion_type type = COMPLETE_FILES_AND_DIRS;
    completion_entry *ce = input_lookup_completion(argv[0]);
    if (ce)
        type = ce->type;

    /* ============================================================
       Determine which argument to complete
       Rules:
         - If only command: complete empty second arg
         - If flags present: complete LAST non-flag argument
         - Else: complete argv[1]
       ============================================================ */

    int target_index = 1;

    if (argc == 1)
    {
        /* "ls<TAB>" → complete empty second arg */
        argv[1] = "";
        target_index = 1;
    }
    else
    {
        /* If argv[1] is a flag, complete the last argument */
        if (argv[1][0] == '-' && argc > 2)
        {
            target_index = argc - 1;
        }
        else
        {
            /* Normal case: complete argv[1] */
            target_index = 1;
        }
    }

    /* Build a minimal argv view for completion */
    char *comp_argv[2];
    comp_argv[0] = argv[0];
    comp_argv[1] = argv[target_index];

    /* Save old buffer for change detection */
    char oldbuf[PATH_MAX];
    snprintf(oldbuf, sizeof(oldbuf), "%s", buf);

    /* Perform completion */
    input_complete_path(comp_argv, len, buf, type);

    /* ============================================================
       Redraw ONLY if buffer changed
       ============================================================ */
    if (strcmp(oldbuf, buf) != 0)
    {
        printf("\033[%d;%dH", terminal_rows, PROMPT_COL);
        for (size_t i = 0; i < *len; i++)
            terminal_slow_putc(buf[i]);

        printf("\033[%d;%dH", terminal_rows, PROMPT_COL + (int)*len);
        fflush(stdout);
    }
}

static int input_read_char(void)
{
    int c = getchar();

    if (c != 27)   // not ESC
        return c;

    int c1 = getchar();
    if (c1 != '[') {
        // Not an arrow sequence: put it back and return ESC
        ungetc(c1, stdin);
        return 27;
    }

    int c2 = getchar();

    if (c2 == 'A') return KEY_UP;
    if (c2 == 'B') return KEY_DOWN;

    ungetc(c2, stdin);
    ungetc('[', stdin);
    return 27;
}

void input_read_line(char *buf, size_t maxlen)
{
    size_t len = 0;
    int ch;

    terminal_draw_prompt();
    printf("\033[%d;%dH", terminal_rows, PROMPT_COL);

    while (1) {
        ch = input_read_char();

        switch (ch) {

        case '\n':
        case '\r':
            buf[len] = '\0';
            terminal_draw_prompt();
            return;

        case 127:      // BACKSPACE
        case '\b':
            if (len > 0) {
                len--;
                printf("\033[%d;%dH", terminal_rows, PROMPT_COL + (int)len);
                terminal_slow_putc(' ');
                printf("\033[%d;%dH", terminal_rows, PROMPT_COL + (int)len);
                fflush(stdout);
            }
            break;

        case '\t':     // TAB COMPLETION
            input_handle_tab(buf, &len);
            // after completion, just move cursor to end of buffer and show any new chars
            printf("\033[%d;%dH", terminal_rows, PROMPT_COL);
            for (size_t i = 0; i < len; i++)
                terminal_slow_putc(buf[i]);
            printf("\033[%d;%dH", terminal_rows, PROMPT_COL + (int)len);
            fflush(stdout);
            break;

        // ============================
        //        HISTORY KEYS
        // ============================
        case KEY_UP:
            history_prev(buf, maxlen);
            len = strlen(buf);

            terminal_draw_prompt();
            printf("\033[%d;%dH", terminal_rows, PROMPT_COL);
            printf("%s", buf);
            fflush(stdout);
            break;

        case KEY_DOWN:
            history_next(buf, maxlen);
            len = strlen(buf);

            terminal_draw_prompt();
            printf("\033[%d;%dH", terminal_rows, PROMPT_COL);
            printf("%s", buf);
            fflush(stdout);
            break;

        // ============================
        //        NORMAL CHAR
        // ============================
        default:
            if (len + 1 < maxlen && ch >= 32 && ch < 127) {
                buf[len++] = (char)ch;
                printf("\033[%d;%dH", terminal_rows, PROMPT_COL + (int)(len - 1));
                terminal_slow_putc((char)ch);
            }
            break;
        }
    }
}