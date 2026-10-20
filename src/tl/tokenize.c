#include "main.h"
#include "tl.h"

tl_token_t *tl_tokenize(const char *line)
{
    LOG("tl_tokenize: start '%s'", line);

    const char *p = line;
    tl_token_t *head = NULL;
    tl_token_t *tail = NULL;

    while (*p)
    {
        /* Skip whitespace */
        while (*p && isspace((unsigned char)*p))
            p++;

        if (!*p)
            break;

        /* Operators first */

        /* OR '||' */
        if (p[0] == '|' && p[1] == '|')
        {
            LOG("tl_tokenize: OR '||'");
            tl_token_t *t = calloc(1, sizeof(tl_token_t));
            t->type = TL_TOK_OR;
            t->text = strdup("||");
            if (!head)
                head = t;
            else
                tail->next = t;
            tail = t;
            p += 2;
            continue;
        }

        /* AND '&&' */
        if (p[0] == '&' && p[1] == '&')
        {
            LOG("tl_tokenize: AND '&&'");
            tl_token_t *t = calloc(1, sizeof(tl_token_t));
            t->type = TL_TOK_AND;
            t->text = strdup("&&");
            if (!head)
                head = t;
            else
                tail->next = t;
            tail = t;
            p += 2;
            continue;
        }

        /* APPEND '>>' */
        if (p[0] == '>' && p[1] == '>')
        {
            LOG("tl_tokenize: APPEND '>>'");
            tl_token_t *t = calloc(1, sizeof(tl_token_t));
            t->type = TL_TOK_REDIR_APPEND;
            t->text = strdup(">>");
            if (!head)
                head = t;
            else
                tail->next = t;
            tail = t;
            p += 2;
            continue;
        }

        /* PIPE '|' */
        if (*p == '|')
        {
            LOG("tl_tokenize: PIPE '|'");
            tl_token_t *t = calloc(1, sizeof(tl_token_t));
            t->type = TL_TOK_PIPE;
            t->text = strdup("|");
            if (!head)
                head = t;
            else
                tail->next = t;
            tail = t;
            p++;
            continue;
        }

        /* REDIR_OUT '>' */
        if (*p == '>')
        {
            LOG("tl_tokenize: REDIR_OUT '>'");
            tl_token_t *t = calloc(1, sizeof(tl_token_t));
            t->type = TL_TOK_REDIR_OUT;
            t->text = strdup(">");
            if (!head)
                head = t;
            else
                tail->next = t;
            tail = t;
            p++;
            continue;
        }

        /* REDIR_IN '<' */
        if (*p == '<')
        {
            LOG("tl_tokenize: REDIR_IN '<'");
            tl_token_t *t = calloc(1, sizeof(tl_token_t));
            t->type = TL_TOK_REDIR_IN;
            t->text = strdup("<");
            if (!head)
                head = t;
            else
                tail->next = t;
            tail = t;
            p++;
            continue;
        }

        /* SEMI ';' */
        if (*p == ';')
        {
            LOG("tl_tokenize: SEMI ';'");
            tl_token_t *t = calloc(1, sizeof(tl_token_t));
            t->type = TL_TOK_SEMI;
            t->text = strdup(";");
            if (!head)
                head = t;
            else
                tail->next = t;
            tail = t;
            p++;
            continue;
        }

        /* LPAREN '(' */
        if (*p == '(')
        {
            LOG("tl_tokenize: LPAREN '('");
            tl_token_t *t = calloc(1, sizeof(tl_token_t));
            t->type = TL_TOK_LPAREN;
            t->text = strdup("(");
            if (!head)
                head = t;
            else
                tail->next = t;
            tail = t;
            p++;
            continue;
        }

        /* RPAREN ')' */
        if (*p == ')')
        {
            LOG("tl_tokenize: RPAREN ')'");
            tl_token_t *t = calloc(1, sizeof(tl_token_t));
            t->type = TL_TOK_RPAREN;
            t->text = strdup(")");
            if (!head)
                head = t;
            else
                tail->next = t;
            tail = t;
            p++;
            continue;
        }

        /* WORD (with quoting + escaping) */
        char buf[PATH_MAX];
        int bi = 0;

        if (*p == '"')
        {
            p++;
            while (*p && *p != '"')
            {
                if (*p == '\\' && p[1])
                {
                    buf[bi++] = p[1];
                    p += 2;
                    continue;
                }
                buf[bi++] = *p++;
            }
            if (*p == '"')
                p++;
        }
        else if (*p == '\'')
        {
            p++;
            while (*p && *p != '\'')
            {
                buf[bi++] = *p++;
            }
            if (*p == '\'')
                p++;
        }
        else
        {
            while (*p && !isspace((unsigned char)*p) &&
                   *p != '|' && *p != '&' && *p != ';' &&
                   *p != '<' && *p != '>' &&
                   *p != '(' && *p != ')')
            {
                if (*p == '\\' && p[1])
                {
                    buf[bi++] = p[1];
                    p += 2;
                    continue;
                }
                buf[bi++] = *p++;
            }
        }

        buf[bi] = '\0';

        LOG("tl_tokenize: WORD '%s'", buf);

        tl_token_t *t = calloc(1, sizeof(tl_token_t));
        t->type = TL_TOK_WORD;
        t->text = strdup(buf);

        if (!head)
            head = t;
        else
            tail->next = t;
        tail = t;
    }

    LOG("tl_tokenize: done");
    return head;
}
