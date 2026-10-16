#ifndef HISTORY_H
#define HISTORY_H

#define HISTORY_MAX 256
#define HISTORY_FILE ".termlink_history"

typedef struct {
    char history[HISTORY_MAX][PATH_MAX]; // loaded history
    int history_count;                   // current history count
    char history_path[PATH_MAX];         // history path

    int history_index;                   // current history position
    int history_edit_saved;              // whether original buffer is saved
    char history_edit_buf[PATH_MAX];     // original buffer before history nav
} history_entry;

void history_load(history_entry *hist);
void history_save(history_entry *hist);
void history_add(history_entry *hist, const char *cmd);
void history_prev(history_entry *hist, char *buf, size_t maxlen);
void history_next(history_entry *hist, char *buf, size_t maxlen);

#endif