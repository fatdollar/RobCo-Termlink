#include "main.h"
#include "history.h"

static char history[HISTORY_MAX][PATH_MAX];
static int history_count = 0;
static int history_index = 0;
static char history_path[PATH_MAX];

static void history_init_path(void)
{
    const char *home = getenv("HOME");
    if (!home) {
        history_path[0] = '\0';
        return;
    }

    snprintf(history_path, sizeof(history_path), "%s/%s", home, HISTORY_FILE);
}

void history_load(void)
{
    history_init_path();
    if (history_path[0] == '\0')
        return;

    FILE *fp = fopen(history_path, "r");
    if (!fp)
        return;

    char buf[PATH_MAX];

    while (fgets(buf, sizeof(buf), fp)) {
        buf[strcspn(buf, "\n")] = '\0'; // strip newline

        if (history_count < HISTORY_MAX && buf[0] != '\0') {
            snprintf(history[history_count], PATH_MAX, "%s", buf);
            history_count++;
        }
    }

    fclose(fp);
    history_index = history_count;
}

void history_save(void)
{
    history_init_path();
    if (history_path[0] == '\0')
        return;

    FILE *fp = fopen(history_path, "w");
    if (!fp)
        return;

    for (int i = 0; i < history_count; i++)
        fprintf(fp, "%s\n", history[i]);

    fclose(fp);
}

void history_add(const char *cmd)
{
    if (!cmd || !*cmd)
        return;

    // Avoid duplicate consecutive entries
    if (history_count > 0 && strcmp(history[history_count - 1], cmd) == 0)
        return;

    if (history_count < HISTORY_MAX) {
        snprintf(history[history_count], PATH_MAX, "%s", cmd);
        history_count++;
    } else {
        for (int i = 1; i < HISTORY_MAX; i++)
            memmove(history[i - 1], history[i], PATH_MAX);

        snprintf(history[HISTORY_MAX - 1], PATH_MAX, "%s", cmd);
    }

    history_index = history_count; // reset to empty prompt
}

void history_prev(char *buf, size_t maxlen)
{
    if (history_count == 0)
        return;

    // If we're at the empty prompt, jump to last entry
    if (history_index == history_count)
        history_index = history_count - 1;
    else if (history_index > 0)
        history_index--;

    snprintf(buf, maxlen, "%s", history[history_index]);
}

void history_next(char *buf, size_t maxlen)
{
    if (history_count == 0)
        return;

    if (history_index < history_count - 1) {
        history_index++;
        snprintf(buf, maxlen, "%s", history[history_index]);
    } else {
        // Move to empty prompt
        history_index = history_count;
        buf[0] = '\0';
    }
}