#include "parser.h"
#include "../utils/string_utils.h"
#include <stdlib.h>
#include <string.h>

ASTNode* cax_template_parse(const char *source) {
    Lexer lexer;
    cax_lexer_init(&lexer, source);
    
    ASTNode *head = NULL;
    ASTNode *tail = NULL;
    
    Token token = cax_lexer_next_token(&lexer);
    while (token.type != TOKEN_EOF) {
        if (token.type == TOKEN_TEXT || token.type == TOKEN_VAR_OPEN) {
            ASTNode *node = malloc(sizeof(ASTNode));
            node->type = (token.type == TOKEN_TEXT) ? NODE_TEXT : NODE_VAR;
            
            if (token.type == TOKEN_VAR_OPEN) {
                Token var_token = cax_lexer_next_token(&lexer);
                if (var_token.type == TOKEN_TEXT) {
                    node->value = malloc(var_token.length + 1);
                    memcpy(node->value, var_token.start, var_token.length);
                    node->value[var_token.length] = '\0';
                    node->value = cax_strtrim(node->value);
                } else {
                    node->value = cax_strdup("");
                }
                cax_lexer_next_token(&lexer); // Skip TOKEN_VAR_CLOSE
            } else {
                node->value = malloc(token.length + 1);
                memcpy(node->value, token.start, token.length);
                node->value[token.length] = '\0';
            }
            
            node->left = NULL;
            node->right = NULL;
            node->next = NULL;
            
            if (!head) {
                head = node;
                tail = node;
            } else {
                tail->next = node;
                tail = node;
            }
        }
        token = cax_lexer_next_token(&lexer);
    }
    
    return head;
}

void cax_ast_free(ASTNode *node) {
    while (node) {
        ASTNode *next = node->next;
        free(node->value);
        if (node->left) cax_ast_free(node->left);
        if (node->right) cax_ast_free(node->right);
        free(node);
        node = next;
    }
}