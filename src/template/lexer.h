#ifndef LEXER_H
#define LEXER_H

typedef enum {
    TOKEN_EOF,
    TOKEN_TEXT,
    TOKEN_TAG_OPEN,     // {%
    TOKEN_TAG_CLOSE,    // %}
    TOKEN_VAR_OPEN,     // {{
    TOKEN_VAR_CLOSE,    // }}
    TOKEN_IDENTIFIER,
    TOKEN_KEYWORD_IF,
    TOKEN_KEYWORD_FOR,
    TOKEN_KEYWORD_ENDIF,
    TOKEN_KEYWORD_ENDFOR,
    TOKEN_STRING,
    TOKEN_DOT
} TokenType;

typedef struct {
    TokenType type;
    const char *start;
    size_t length;
} Token;

typedef struct {
    const char *source;
    const char *current;
} Lexer;

void cax_lexer_init(Lexer *lexer, const char *source);
Token cax_lexer_next_token(Lexer *lexer);

#endif