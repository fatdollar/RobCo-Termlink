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

void input_complete_path(char **argv, size_t *len, char *buf, completion_type type) {
    const char *prefix = (argv[1]) ? argv[1] : "";

    char dirname[PATH_MAX];
    char partial[PATH_MAX];

    char *slash = strrchr(prefix, '/');
    if (slash) {
        size_t dlen = slash - prefix;
        if (dlen >= sizeof(dirname)) dlen = sizeof(dirname) - 1;
        memcpy(dirname, prefix, dlen);
        dirname[dlen] = '\0';
        snprintf(partial, sizeof(partial), "%s", slash + 1);
    } else {
        snprintf(dirname, sizeof(dirname), ".");
        snprintf(partial, sizeof(partial), "%s", prefix);
    }

    DIR *d = opendir(dirname);
    if (!d) return;

    struct dirent *ent;
    char matches[128][PATH_MAX];
    int match_count = 0;

    while ((ent = readdir(d)) != NULL) {
        if (strncmp(ent->d_name, partial, strlen(partial)) != 0)
            continue;

        size_t need = strlen(dirname) + strlen(ent->d_name) + 2;
        char fullpath[need];
        snprintf(fullpath, need, "%s/%s", dirname, ent->d_name);

        struct stat st;
        if (stat(fullpath, &st) != 0) continue;

        if (type == COMPLETE_DIRS && !S_ISDIR(st.st_mode)) continue;
        if (type == COMPLETE_FILES && !S_ISREG(st.st_mode)) continue;

        snprintf(matches[match_count++], PATH_MAX, "%s", ent->d_name);
        if (match_count >= 128) break;
    }
    closedir(d);

    if (match_count == 0) return;

    if (match_count == 1) {
        const char *m = matches[0];

        size_t plen = strlen(prefix);
        size_t mlen = strlen(m);

        *len -= plen;
        buf[*len] = '\0';

        snprintf(buf + *len, PATH_MAX - *len, "%s", m);
        *len += mlen;

        terminal_draw_prompt();
        printf("\033[%d;%dH", terminal_rows, PROMPT_COL);
        for (size_t i = 0; i < *len; i++) terminal_slow_putc(buf[i]);
        return;
    }

    terminal_slow_print("Matches:");
    for (int i = 0; i < match_count; i++) {
        terminal_slow_print(matches[i]);
    }

    terminal_draw_prompt();
    for (size_t i = 0; i < *len; i++) terminal_slow_putc(buf[i]);
}

void input_handle_tab(char *buf, size_t *len) {
	char temp[PATH_MAX];
    char *argv[MAX_ARGS];

    terminal_clear_scroll();
    terminal_cursor_line = terminal_scroll_top;

    snprintf(temp, sizeof(temp), "%s", buf);
    int argc = input_parse_args(temp, argv, MAX_ARGS);
    if (argc == 0) return;

    completion_type type = COMPLETE_FILES_AND_DIRS;
    completion_entry *ce = input_lookup_completion(argv[0]);
    if (ce) type = ce->type;

    if (argc == 1)
        argv[1] = "";

    input_complete_path(argv, len, buf, type);

    // After completion, redraw prompt + buffer
    terminal_draw_prompt();
    printf("\033[%d;%dH", terminal_rows, PROMPT_COL);

    for (size_t i = 0; i < *len; i++)
        terminal_slow_putc(buf[i]);

    fflush(stdout);
}

void input_read_line(char *buf, size_t maxlen) {
    size_t len = 0;
    int ch;

    terminal_draw_prompt();
	printf("\033[%d;%dH", terminal_rows, PROMPT_COL);

	while (1) {
		ch = getchar();

		if (ch == '\n' || ch == '\r') {
			break;
		}

		else if (ch == 127 || ch == '\b') {   // BACKSPACE
			if (len > 0) {
				len--;
				printf("\033[%d;%dH", terminal_rows, PROMPT_COL + (int)len);
				terminal_slow_putc(' ');
				printf("\033[%d;%dH", terminal_rows, PROMPT_COL + (int)len);
				fflush(stdout);
			}
		}

		else if (ch == '\t') {               // TAB COMPLETION
			input_handle_tab(buf, &len);

			// restore cursor to correct position
			printf("\033[%d;%dH", terminal_rows, PROMPT_COL + (int)len);
			fflush(stdout);
			continue;
		}

		else if (len + 1 < maxlen) {         // NORMAL CHAR
			buf[len++] = (char)ch;
			printf("\033[%d;%dH", terminal_rows, PROMPT_COL + (int)(len - 1));
			terminal_slow_putc((char)ch);
		}
	}

    buf[len] = '\0';
    terminal_draw_prompt();
}