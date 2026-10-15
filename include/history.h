#ifndef HISTORY_H
#define HISTORY_H

#include <stddef.h>

void history_add(const char *cmd);
void history_prev(char *buf, size_t maxlen);
void history_next(char *buf, size_t maxlen);
void history_load(void);
void history_save(void);

#endif