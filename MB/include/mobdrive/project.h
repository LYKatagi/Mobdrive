#ifndef MOBDRIVE_PROJECT_H
#define MOBDRIVE_PROJECT_H
#include "lexer.h"
#include "parser.h"

typedef struct ProjectUnit {
    char *path;
    TokenList tokens;
    Program program;
    struct ProjectUnit *next;
} ProjectUnit;
typedef struct Project {
    ProjectUnit *head;
    ProjectUnit **tail;
    int errors;
} Project;

int project_load(Project *project, const char *root_source);
void project_free(Project *project);
#endif
