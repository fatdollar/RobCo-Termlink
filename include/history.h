#ifndef HISTORY_H
#define HISTORY_H

#define HISTORY_MAX 256
#define HISTORY_FILE ".termlink_history"

void history_load(void);
void history_save(void);
void history_add(const char *cmd);
void history_prev(char *buf, size_t maxlen);
void history_next(char *buf, size_t maxlen);

#endif