#ifndef TL_H
#define TL_H

#include "tl_ctx.h"

typedef enum
{
    TL_TOK_WORD,
    TL_TOK_PIPE,
    TL_TOK_OR,
    TL_TOK_AND,
    TL_TOK_SEMI,
    TL_TOK_REDIR_OUT,
    TL_TOK_REDIR_APPEND,
    TL_TOK_REDIR_IN,
    TL_TOK_LPAREN,
    TL_TOK_RPAREN
} tl_tok_kind_t;

typedef struct tl_token
{
    tl_tok_kind_t kind;
    char *text;
    struct tl_token *next;
} tl_token_t;

typedef struct
{
    tl_token_t *tok;
} tl_parse_state_t;

tl_node_t *tl_parse(const char *line);
tl_token_t *tl_tokenize(const char *line);
int tl_exec_line(const char *line, tl_exec_ctx_t *ctx);

#endif