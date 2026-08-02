#include "lexer.h"
#include <string.h>
#include <ctype.h>

void cax_lexer_init(Lexer *lexer, const char *source) {
    lexer->source = source;
    lexer->current = source;
}

Token cax_lexer_next_token(Lexer *lexer) {
    while (*lexer->current != '\0') {
        if (*lexer->current == '{' && *(lexer->current + 1) == '{') {
            Token token = { TOKEN_VAR_OPEN, lexer->current, 2 };
            lexer->current += 2;
            return token;
        }
        
        if (*lexer->current == '}' && *(lexer->current + 1) == '}') {
            Token token = { TOKEN_VAR_CLOSE, lexer->current, 2 };
            lexer->current += 2;
            return token;
        }
        
        if (*lexer->current == '{' && *(lexer->current + 1) == '%') {
            Token token = { TOKEN_TAG_OPEN, lexer->current, 2 };
            lexer->current += 2;
            return token;
        }
        
        if (*lexer->current == '%' && *(lexer->current + 1) == '}') {
            Token token = { TOKEN_TAG_CLOSE, lexer->current, 2 };
            lexer->current += 2;
            return token;
        }
        
        const char *text_start = lexer->current;
        while (*lexer->current != '\0' && !(*lexer->current == '{' && (*(lexer->current + 1) == '{' || *(lexer->current + 1) == '%'))) {
            lexer->current++;
        }
        
        if (lexer->current > text_start) {
            Token token = { TOKEN_TEXT, text_start, (size_t)(lexer->current - text_start) };
            return token;
        }
        
        lexer->current++;
    }
    
    Token token = { TOKEN_EOF, lexer->current, 0 };
    return token;
}