#ifndef TEMPLATE_PARSER_H
#define TEMPLATE_PARSER_H

#include "lexer.h"
#include "../../include/cax.h"

typedef enum {
    NODE_TEXT,
    NODE_VAR,
    NODE_IF,
    NODE_FOR
} NodeType;

struct ASTNode;

typedef struct ASTNode {
    NodeType type;
    char *value;
    struct ASTNode *left;
    struct ASTNode *right;
    struct ASTNode *next;
} ASTNode;

ASTNode* cax_template_parse(const char *source);
void cax_ast_free(ASTNode *node);

#endif