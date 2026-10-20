#include "main.h"
#include "tl.h"

static tl_node_t *_tl_parse_list(tl_parse_state_t *ps);

static int _tl_is_assignment_word(const char *s)
{
    /* NAME=VALUE, where NAME is [A-Za-z_][A-Za-z0-9_]* */
    const char *p = s;

    if (!(*p == '_' || (*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z')))
        return 0;

    p++;
    while (*p && (*p == '_' || (*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9')))
        p++;

    if (*p != '=')
        return 0;

    /* must have something after '=' to be useful, but POSIX allows empty too */
    return 1;
}

static tl_token_t *_tl_peek(tl_parse_state_t *ps)
{
    return ps->tok;
}

static tl_token_t *_tl_next(tl_parse_state_t *ps)
{
    tl_token_t *t = ps->tok;
    if (ps->tok)
        ps->tok = ps->tok->next;
    return t;
}

static int _tl_accept(tl_parse_state_t *ps, tl_tok_type_t type)
{
    if (ps->tok && ps->tok->type == type)
    {
        ps->tok = ps->tok->next;
        return 1;
    }
    return 0;
}

static tl_word_t *_tl_parse_words(tl_parse_state_t *ps)
{
    LOG("_tl_parse_words: start");

    tl_word_t *head = NULL;
    tl_word_t *tail = NULL;

    while (_tl_peek(ps) && _tl_peek(ps)->type == TL_TOK_WORD)
    {
        tl_token_t *t = _tl_next(ps);

        tl_word_t *w = calloc(1, sizeof(tl_word_t));
        w->text = strdup(t->text);

        if (!head)
            head = w;
        else
            tail->next = w;

        tail = w;
    }

    LOG("_tl_parse_words: done");
    return head;
}

static tl_redir_t *_tl_parse_redirs(tl_parse_state_t *ps)
{
    LOG("_tl_parse_redirs: start");

    tl_redir_t *head = NULL;
    tl_redir_t *tail = NULL;

    while (_tl_peek(ps))
    {
        tl_tok_type_t k = _tl_peek(ps)->type;

        int fd = 1;
        int type = -1;

        if (k == TL_TOK_REDIR_OUT)
        {
            LOG("_tl_parse_redirs: '>'");
            type = TL_REDIR_OUT;
            _tl_next(ps);
        }
        else if (k == TL_TOK_REDIR_APPEND)
        {
            LOG("_tl_parse_redirs: '>>'");
            type = TL_REDIR_OUT_APPEND;
            _tl_next(ps);
        }
        else if (k == TL_TOK_REDIR_IN)
        {
            LOG("_tl_parse_redirs: '<'");
            type = TL_REDIR_IN;
            fd = 0;
            _tl_next(ps);
        }
        else
        {
            break;
        }

        if (!_tl_peek(ps) || _tl_peek(ps)->type != TL_TOK_WORD)
        {
            LOG("_tl_parse_redirs: missing filename");
            return head;
        }

        tl_token_t *t = _tl_next(ps);

        tl_redir_t *r = calloc(1, sizeof(tl_redir_t));
        r->fd = fd;
        r->type = type;

        tl_word_t *w = calloc(1, sizeof(tl_word_t));
        w->text = strdup(t->text);
        r->target = w;

        if (!head)
            head = r;
        else
            tail->next = r;

        tail = r;
    }

    LOG("_tl_parse_redirs: done");
    return head;
}

static tl_command_t *_tl_parse_command(tl_parse_state_t *ps)
{
    LOG("_tl_parse_command: start");

    tl_word_t *words = _tl_parse_words(ps);
    if (!words)
    {
        LOG("_tl_parse_command: no words");
        return NULL;
    }

    /* Split leading assignments from argv */
    tl_word_t *assigns_head = NULL;
    tl_word_t *assigns_tail = NULL;
    tl_word_t *argv_head = NULL;
    tl_word_t *argv_tail = NULL;

    tl_word_t *cur = words;
    int in_assign_phase = 1;

    while (cur)
    {
        tl_word_t *next = cur->next;
        cur->next = NULL;

        if (in_assign_phase && _tl_is_assignment_word(cur->text))
        {
            LOG("_tl_parse_command: assignment '%s'", cur->text);

            if (!assigns_head)
                assigns_head = cur;
            else
                assigns_tail->next = cur;

            assigns_tail = cur;
        }
        else
        {
            in_assign_phase = 0;

            LOG("_tl_parse_command: argv '%s'", cur->text);

            if (!argv_head)
                argv_head = cur;
            else
                argv_tail->next = cur;

            argv_tail = cur;
        }

        cur = next;
    }

    if (!argv_head && !assigns_head)
    {
        LOG("_tl_parse_command: no argv and no assigns");
        return NULL;
    }

    tl_redir_t *redirs = _tl_parse_redirs(ps);

    tl_command_t *cmd = calloc(1, sizeof(tl_command_t));
    cmd->argv = argv_head;
    cmd->redirs = redirs;
    cmd->assignments = assigns_head;

    LOG("_tl_parse_command: done");
    return cmd;
}

static tl_node_t *_tl_parse_subshell(tl_parse_state_t *ps)
{
    LOG("_tl_parse_subshell: start");

    if (!_tl_accept(ps, TL_TOK_LPAREN))
    {
        LOG("_tl_parse_subshell: missing '('");
        return NULL;
    }

    tl_node_t *body = NULL;
    body = _tl_parse_list(ps);
    if (!body)
    {
        LOG("_tl_parse_subshell: body parse failed");
        return NULL;
    }

    if (!_tl_accept(ps, TL_TOK_RPAREN))
    {
        LOG("_tl_parse_subshell: missing ')'");
        return NULL;
    }

    tl_subshell_t *sub = calloc(1, sizeof(tl_subshell_t));
    sub->body = body->as.list;

    tl_node_t *node = calloc(1, sizeof(tl_node_t));
    node->type = TL_NODE_SUBSHELL;
    node->as.subshell = sub;

    LOG("_tl_parse_subshell: done");
    return node;
}

static tl_node_t *_tl_parse_pipeline(tl_parse_state_t *ps)
{
    LOG("_tl_parse_pipeline: start");

    tl_node_t *first = NULL;

    /* First element: command or subshell */
    if (_tl_peek(ps) && _tl_peek(ps)->type == TL_TOK_LPAREN)
    {
        first = _tl_parse_subshell(ps);
        if (!first)
        {
            LOG("_tl_parse_pipeline: subshell parse failed");
            return NULL;
        }
    }
    else
    {
        tl_command_t *cmd = _tl_parse_command(ps);
        if (!cmd)
        {
            LOG("_tl_parse_pipeline: no first command");
            return NULL;
        }

        first = calloc(1, sizeof(tl_node_t));
        first->type = TL_NODE_COMMAND;
        first->as.command = cmd;
    }

    /* If no pipe, return the single node */
    if (!_tl_peek(ps) || _tl_peek(ps)->type != TL_TOK_PIPE)
    {
        LOG("_tl_parse_pipeline: single command");
        return first;
    }

    /* Pipeline detected */
    LOG("_tl_parse_pipeline: pipeline detected");

    /* Create pipeline struct */
    tl_pipeline_t *pl = calloc(1, sizeof(tl_pipeline_t));
    pl->count = 0;

    /* Dynamic array for commands */
    int cap = 4;
    pl->commands = calloc(cap, sizeof(tl_command_t *));

    /* First element must be a command (your AST stores commands only) */
    if (first->type != TL_NODE_COMMAND)
    {
        LOG("_tl_parse_pipeline: subshell in pipeline not supported");
        return first;
    }

    pl->commands[pl->count++] = first->as.command;

    /* Parse remaining pipeline segments */
    while (_tl_accept(ps, TL_TOK_PIPE))
    {
        LOG("_tl_parse_pipeline: '|'");

        tl_command_t *cmd = _tl_parse_command(ps);
        if (!cmd)
        {
            LOG("_tl_parse_pipeline: missing command after '|'");
            break;
        }

        /* Grow array if needed */
        if (pl->count >= cap)
        {
            cap *= 2;
            pl->commands = realloc(pl->commands, cap * sizeof(tl_command_t *));
        }

        pl->commands[pl->count++] = cmd;
    }

    /* Wrap pipeline in a node */
    tl_node_t *node = calloc(1, sizeof(tl_node_t));
    node->type = TL_NODE_PIPELINE;
    node->as.pipeline = pl;

    LOG("_tl_parse_pipeline: done, count=%d", pl->count);
    return node;
}

static tl_node_t *_tl_parse_list(tl_parse_state_t *ps)
{
    LOG("_tl_parse_list: start");

    tl_list_t *list = calloc(1, sizeof(tl_list_t));
    tl_list_item_t *head = NULL;
    tl_list_item_t *tail = NULL;

    while (_tl_peek(ps))
    {
        tl_node_t *node = _tl_parse_pipeline(ps);
        if (!node)
        {
            LOG("_tl_parse_list: pipeline parse failed");
            break;
        }

        tl_list_item_t *it = calloc(1, sizeof(tl_list_item_t));
        it->node = node;

        if (!head)
            head = it;
        else
            tail->next = it;

        tail = it;

        if (!_tl_peek(ps))
            break;

        if (_tl_accept(ps, TL_TOK_SEMI))
        {
            it->op_to_next = TL_LIST_SEQ;
            LOG("_tl_parse_list: ';' -> SEQ");
        }
        else if (_tl_accept(ps, TL_TOK_AND))
        {
            it->op_to_next = TL_LIST_AND;
            LOG("_tl_parse_list: '&&' -> AND");
        }
        else if (_tl_accept(ps, TL_TOK_OR))
        {
            it->op_to_next = TL_LIST_OR;
            LOG("_tl_parse_list: '||' -> OR");
        }
        else
        {
            LOG("_tl_parse_list: no list operator, stopping");
            break;
        }
    }

    list->head = head;

    tl_node_t *node = calloc(1, sizeof(tl_node_t));
    node->type = TL_NODE_LIST;
    node->as.list = list;

    LOG("_tl_parse_list: done");
    return node;
}

tl_node_t *tl_parse(const char *line)
{
    LOG("tl_parse: '%s'", line);

    tl_token_t *tok = tl_tokenize(line);
    if (!tok)
    {
        LOG("tl_parse: no tokens");
        return NULL;
    }

    tl_parse_state_t ps;
    ps.tok = tok;

    tl_node_t *root = _tl_parse_list(&ps);

    if (ps.tok != NULL)
    {
        LOG("tl_parse: trailing tokens after parse (type=%d)", ps.tok->type);
        /* You can choose to treat this as error: */
        /* return NULL; */
    }

    LOG("tl_parse: done");
    return root;
}