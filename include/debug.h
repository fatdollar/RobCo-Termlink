#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#ifdef TERMLINK_DEBUG

extern FILE *debug_log;

// Initialize logging system
void debug_init(void);

// Core logging macro
#define LOG(fmt, ...) \
    do { \
        if (debug_log) { \
            fprintf(debug_log, "[%s:%d:%s] " fmt "\n", \
                __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
            fflush(debug_log); \
        } \
    } while (0)

// Category logging
#define LOG_INPUT(fmt, ...)   LOG("INPUT: " fmt, ##__VA_ARGS__)
#define LOG_READ(fmt, ...)    LOG("READ: " fmt, ##__VA_ARGS__)
#define LOG_HISTORY(fmt, ...) LOG("HIST: " fmt, ##__VA_ARGS__)
#define LOG_REDRAW(fmt, ...)  LOG("REDRAW: " fmt, ##__VA_ARGS__)
#define LOG_CURSOR(fmt, ...)  LOG("CURSOR: " fmt, ##__VA_ARGS__)
#define LOG_TOKEN(fmt, ...)   LOG("TOKEN: " fmt, ##__VA_ARGS__)

#else

// Disabled: zero overhead
#define LOG(...)
#define LOG_INPUT(...)
#define LOG_READ(...)
#define LOG_HISTORY(...)
#define LOG_REDRAW(...)
#define LOG_CURSOR(...)
#define LOG_TOKEN(...)

static inline void debug_init(void) {}

#endif

#endif
