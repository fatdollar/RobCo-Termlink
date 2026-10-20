#ifndef TL_H
#define TL_H

#include "tl_struct.h"

tl_node_t *tl_parse(const char *line);
tl_token_t *tl_tokenize(const char *line);
int tl_exec_line(const char *line, tl_exec_ctx_t *ctx);

#endif