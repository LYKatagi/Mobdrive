#include <mobdrive/lexer.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int add(TokenList *l, TokenKind kind, const char *s, size_t n, unsigned line) {
    if (l->count == l->capacity) {
        size_t cap = l->capacity ? l->capacity * 2 : 128;
        Token *p = (Token *)realloc(l->items, cap * sizeof(*p));
        if (!p) return 0;
        l->items = p; l->capacity = cap;
    }
    char *text = (char *)malloc(n + 1);
    if (!text) return 0;
    memcpy(text, s, n); text[n] = '\0';
    l->items[l->count++] = (Token){kind, text, line};
    return 1;
}
int lexer_tokenize(const char *s, const char *filename, TokenList *out) {
    size_t i = 0; unsigned line = 1; memset(out, 0, sizeof(*out)); out->filename = filename;
    while (s[i]) {
        unsigned char c = (unsigned char)s[i];
        if (c == '\n') { line++; i++; continue; }
        if (isspace(c)) { i++; continue; }
        if (s[i]=='/' && s[i+1]=='/') { while(s[i] && s[i]!='\n') i++; continue; }
        if (s[i]=='/' && s[i+1]=='*') {
            i+=2;
            while(s[i] && !(s[i]=='*' && s[i+1]=='/')) { if(s[i]=='\n') line++; i++; }
            if(s[i]) i+=2; else {fprintf(stderr,"%s:%u: error: unterminated block comment\n",filename,line);out->errors++;}
            continue;
        }
        size_t start=i;
        if (isalpha(c)||c=='_') { i++; while(isalnum((unsigned char)s[i])||s[i]=='_')i++; if(!add(out,TOK_ID,s+start,i-start,line))goto oom; continue; }
        if (isdigit(c)) { i++; while(isalnum((unsigned char)s[i])||s[i]=='.'||s[i]=='_')i++; if(!add(out,TOK_NUMBER,s+start,i-start,line))goto oom; continue; }
        if (c=='"'||c=='\'') {
            char quote=(char)c; i++;
            while(s[i] && s[i]!=quote) { if(s[i]=='\\'&&s[i+1])i++; if(s[i]=='\n')line++; i++; }
            if(!s[i]) {fprintf(stderr,"%s:%u: error: unterminated string literal\n",filename,line);out->errors++;} else i++;
            if(!add(out,TOK_STRING,s+start,i-start,line))goto oom;
            continue;
        }
        static const char *pairs[]={"->","::","==","!=","<=",">=","&&","||","++","--","+=","-=","*=","/="};
        int pair=0;
        for(size_t k=0;k<sizeof(pairs)/sizeof(pairs[0]);k++) if(s[i]==pairs[k][0]&&s[i+1]==pairs[k][1]){i+=2;pair=1;break;}
        if(!pair)i++;
        if(!add(out,TOK_SYMBOL,s+start,i-start,line))goto oom;
    }
    if(!add(out,TOK_EOF,"<eof>",5,line))goto oom;
    return 1;
oom: fprintf(stderr,"%s: error: out of memory while tokenizing\n",filename); lexer_free(out); return 0;
}
void lexer_free(TokenList *l) { for(size_t i=0;i<l->count;i++)free(l->items[i].text); free(l->items); memset(l,0,sizeof(*l)); }
