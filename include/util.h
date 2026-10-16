#ifndef UTIL_H
#define UTIL_H

#include "input.h"
#include "autocomplete.h"

int util_is_executable(const char *path);

autocomplete_type util_parse_completion_type(const char *value);
void util_register_completion(const char *cmd, autocomplete_type type);

void util_load_completions_dir(const char *dirpath);
void util_load_completions(void);

#endif