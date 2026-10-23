#ifndef MOBDRIVE_LEXER_H
#define MOBDRIVE_LEXER_H
#include <stddef.h>

typedef enum { TOK_EOF, TOK_ID, TOK_NUMBER, TOK_STRING, TOK_SYMBOL } TokenKind;
typedef struct { TokenKind kind; char *text; unsigned line; } Token;
typedef struct { Token *items; size_t count, capacity; const char *filename; int errors; } TokenList;

int lexer_tokenize(const char *source, const char *filename, TokenList *out);
void lexer_free(TokenList *tokens);
#endif
