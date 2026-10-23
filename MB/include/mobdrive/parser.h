#ifndef MOBDRIVE_PARSER_H
#define MOBDRIVE_PARSER_H
#include "lexer.h"

typedef struct Expr { char *text, *inferred_type; } Expr;
typedef struct Param { char *type, *name; struct Param *next; } Param;
typedef enum { ST_VAR, ST_PRINT, ST_RETURN, ST_IF, ST_WHILE, ST_EXPR, ST_BLOCK } StmtKind;
typedef struct Stmt {
    StmtKind kind; char *name, *type; int is_const, infer_type, is_unsafe; Expr expr, cond;
    struct Stmt *body, *otherwise, *next;
} Stmt;
typedef struct Function {
    char *name, *return_type, *link_mode, *owner; int is_external, is_method, is_interface_method, is_visual;
    Param *params; Stmt *body; struct Function *next;
} Function;
typedef struct Field { char *name, *type; int is_const; struct Field *next; } Field;
typedef struct NameRef { char *name; struct NameRef *next; } NameRef;
typedef struct ClassDecl {
    char *name, *base_name; int is_graphic; Field *fields; Function *methods; NameRef *interfaces;
    struct ClassDecl *next;
} ClassDecl;
typedef struct InterfaceDecl {
    char *name; Function *methods; struct InterfaceDecl *next;
} InterfaceDecl;
typedef struct Import { char *source_path, *artifact_path; struct Import *next; } Import;
typedef struct CheckRule { char **names; size_t count; char *level; struct CheckRule *next; } CheckRule;
typedef struct Program {
    Stmt *globals, **globals_tail;
    Stmt *module_stmts, **module_stmts_tail;
    Function *functions, **functions_tail;
    ClassDecl *classes, **classes_tail;
    InterfaceDecl *interfaces, **interfaces_tail;
    Import *imports, **imports_tail;
    CheckRule *checks, **checks_tail;
} Program;

int parser_parse(const TokenList *tokens, Program *program);
void parser_free(Program *program);
#endif
