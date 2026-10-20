#ifndef TL_STRUCT_H
#define TL_STRUCT_H

/* Top Level Nodes */
typedef enum
{
    TL_NODE_COMMAND,
    TL_NODE_PIPELINE,
    TL_NODE_LIST,
    TL_NODE_SUBSHELL,
    TL_NODE_IF,
    TL_NODE_WHILE,
    TL_NODE_FOR,
    TL_NODE_CASE,
    TL_NODE_FUNCTION
} tl_node_type;

/* Node */
typedef struct tl_node
{
    tl_node_type type;
    struct tl_node *next;
    union
    {
        struct tl_command *command;
        struct tl_pipeline *pipeline;
        struct tl_list *list;
        struct tl_subshell *subshell;
    } as;
} tl_node_t;

/* Word */
typedef struct tl_word
{
    char *text;
    struct tl_word *next;
} tl_word_t;

/* Redirection */
typedef struct tl_redir
{
    int fd;
    enum
    {
        TL_REDIR_OUT,
        TL_REDIR_OUT_APPEND,
        TL_REDIR_IN,
        TL_REDIR_ERR_TO_OUT
    } kind;
    tl_word_t *target;
    struct tl_redir *next;
} tl_redir_t;

/* Command */
typedef struct tl_command
{
    tl_word_t *assignments;
    tl_word_t *argv;
    tl_redir_t *redirs;
} tl_command_t;

/* Pipeline */
typedef struct tl_pipeline
{
    tl_command_t **commands;
    int count;
    int negated;
} tl_pipeline_t;

/* List operators */
typedef enum
{
    TL_LIST_SEQ,
    TL_LIST_AND,
    TL_LIST_OR
} tl_list_op_t;

/* List item */
    typedef struct tl_list_item
{
    tl_node_t *node;
    tl_list_op_t op_to_next;
    struct tl_list_item *next;
} tl_list_item_t;

/* List */
typedef struct tl_list
{
    tl_list_item_t *head;
} tl_list_t;

/* Subshell */
typedef struct tl_subshell
{
    tl_list_t *body;
} tl_subshell_t;

#endif