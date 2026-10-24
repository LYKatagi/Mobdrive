#include <mobdrive/typecheck.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct Symbol { const char *name, *type; struct Symbol *next; } Symbol;
typedef struct { Project *project; ProjectUnit *unit; TokenList tokens; size_t at; Symbol *symbols; const char *return_type; const char *owner; const char *name_space; char namespace_storage[512]; int unsafe_depth, loop_depth, errors; } Check;
static char *copy_type(const char*s){size_t n=strlen(s)+1;char*r=(char*)malloc(n);if(r)memcpy(r,s,n);return r;}

static Token *cur(Check *c){return &c->tokens.items[c->at];}
static int is(Check*c,const char*s){return cur(c)->kind!=TOK_EOF&&!strcmp(cur(c)->text,s);}
static int accept(Check*c,const char*s){if(is(c,s)){c->at++;return 1;}return 0;}
static void err(Check*c,const char*m){fprintf(stderr,"%s: type error: %s (near '%s')\n",c->unit->path,m,cur(c)->text);c->errors++;}
static int numeric(const char*t){return t&&(!strcmp(t,"int")||!strcmp(t,"char")||!strcmp(t,"int8_t")||!strcmp(t,"int16_t")||!strcmp(t,"int32_t")||!strcmp(t,"int64_t")||!strcmp(t,"uint8_t")||!strcmp(t,"uint16_t")||!strcmp(t,"uint32_t")||!strcmp(t,"uint64_t")||!strcmp(t,"float")||!strcmp(t,"double"));}
static int integral(const char*t){return numeric(t)&&strcmp(t,"float")&&strcmp(t,"double");}
static int pointer(const char*t){return t&&strstr(t,"*")!=NULL;}
static int scalar(const char*t){return numeric(t)||(t&&!strcmp(t,"bool"))||pointer(t);}
static const char *base_pointer(const char*t){static char b[256];size_t n=t?strlen(t):0;if(!n||n>=sizeof b)return "int32_t";memcpy(b,t,n+1);char*p=strstr(b,"*");if(p){while(p>b&&isspace((unsigned char)p[-1]))p--;*p=0;}return b;}
static int compatible(const char*a,const char*b){if(!a||!b)return 0;if(!strcmp(a,b))return 1;if(numeric(a)&&numeric(b))return 1;if(pointer(a)&&!strcmp(b,"void *"))return 1;if(pointer(b)&&!strcmp(a,"void *"))return 1;if(pointer(a)&&pointer(b)&&(!strcmp(base_pointer(a),"void")||!strcmp(base_pointer(b),"void")))return 1;return 0;}
static void push(Check*c,const char*n,const char*t){Symbol*s=(Symbol*)malloc(sizeof(*s));if(!s)return;s->name=n;s->type=t;s->next=c->symbols;c->symbols=s;}
static void pop_to(Check*c,Symbol*mark){while(c->symbols!=mark){Symbol*s=c->symbols;c->symbols=s->next;free(s);}}
static const char *lookup(Check*c,const char*n){for(Symbol*s=c->symbols;s;s=s->next)if(!strcmp(s->name,n))return s->type;return NULL;}
static Function *find_function(Project*p,const char*n){for(ProjectUnit*u=p->head;u;u=u->next)for(Function*f=u->program.functions;f;f=f->next)if(!strcmp(f->name,n))return f;return NULL;}
static ClassDecl *find_class(Project*p,const char*n){for(ProjectUnit*u=p->head;u;u=u->next)for(ClassDecl*k=u->program.classes;k;k=k->next)if(!strcmp(k->name,n))return k;return NULL;}
static Field *find_field(Project*p,const char*type,const char*n){for(ClassDecl*k=find_class(p,type);k;k=k->base_name?find_class(p,k->base_name):NULL)for(Field*f=k->fields;f;f=f->next)if(!strcmp(f->name,n))return f;return NULL;}
static Function *find_method(Project*p,const char*type,const char*n){for(ClassDecl*k=find_class(p,type);k;k=k->base_name?find_class(p,k->base_name):NULL)for(Function*f=k->methods;f;f=f->next)if(!strcmp(f->name,n))return f;return NULL;}
static Function *resolved(Check*c,const char*n){if(strstr(n,"::"))return find_function(c->project,n);const char*scope=c->owner?c->owner:c->name_space;if(scope){size_t a=strlen(scope),b=strlen(n);char*q=(char*)malloc(a+b+3);if(q){memcpy(q,scope,a);memcpy(q+a,"::",2);memcpy(q+a+2,n,b+1);Function*f=find_function(c->project,q);free(q);if(f)return f;}}return find_function(c->project,n);}
static int precedence(const char*o){if(!strcmp(o,"=" )||!strcmp(o,"+=" )||!strcmp(o,"-="))return 1;if(!strcmp(o,"||"))return 2;if(!strcmp(o,"&&"))return 3;if(!strcmp(o,"==")||!strcmp(o,"!=")||!strcmp(o,"<")||!strcmp(o,">")||!strcmp(o,"<=")||!strcmp(o,">="))return 4;if(!strcmp(o,"+")||!strcmp(o,"-"))return 5;if(!strcmp(o,"*")||!strcmp(o,"/")||!strcmp(o,"%"))return 6;return 0;}
static const char *expression(Check*c,int minp);
static const char *call_arguments(Check*c,Function*f){Param*p=f?f->params:NULL;if(!is(c,")")){do{const char*a=expression(c,1);if(p){if(!compatible(p->type,a))err(c,"function argument type does not match parameter");p=p->next;}else if(f)err(c,"too many function arguments");}while(accept(c,","));}if(!accept(c,")"))err(c,"expected ')' after arguments");if(f&&p)err(c,"too few function arguments");return f?f->return_type:"int32_t";}
static const char *primary(Check*c){
    if(accept(c,"(")){const char*t=expression(c,1);if(!accept(c,")"))err(c,"expected ')' in expression");return t;}
    if(accept(c,"new")){char name[512];size_t n=0;while(cur(c)->kind==TOK_ID&&n+1<sizeof name){size_t z=strlen(cur(c)->text);if(n+z+3>=sizeof name)break;memcpy(name+n,cur(c)->text,z);n+=z;c->at++;if(!accept(c,"::"))break;name[n++]=':';name[n++]=':';}name[n]=0;ClassDecl*k=find_class(c->project,name);if(!k)err(c,"new requires a declared class");if(!accept(c,"("))err(c,"expected '(' after class name in new expression");if(!accept(c,")"))err(c,"constructors and constructor arguments are not supported yet");return k?k->name:copy_type(name);}
    if(accept(c,"&")){const char*t=primary(c);if(!pointer(t)){static char b[256];snprintf(b,sizeof b,"%s *",t?t:"int32_t");return copy_type(b);}return t;}
    if(accept(c,"*")){const char*t=primary(c);if(!pointer(t)){err(c,"cannot dereference a non-pointer value");return "int32_t";}return base_pointer(t);}
    if(accept(c,"!")||accept(c,"~")||accept(c,"+")||accept(c,"-")||accept(c,"++")||accept(c,"--")){const char*op=c->tokens.items[c->at-1].text;const char*t=primary(c);if(!strcmp(op,"!")&&scalar(t))return "bool";if((!strcmp(op,"+")||!strcmp(op,"-"))&&numeric(t))return t;if(!strcmp(op,"~")&&integral(t))return t;if((!strcmp(op,"++")||!strcmp(op,"--"))&&numeric(t))return t;err(c,"invalid unary operator for operand type");return t;}
    Token tok=*cur(c);if(tok.kind==TOK_NUMBER){c->at++;return strchr(tok.text,'.')?"double":"int32_t";}if(tok.kind==TOK_STRING){c->at++;return tok.text[0]=='\''?"char":"const char *";}
    if(accept(c,"true")||accept(c,"false"))return "bool";
    if(accept(c,"null"))return "void *";
    if(tok.kind==TOK_ID){char name[512];size_t n=0;while(cur(c)->kind==TOK_ID&&n+1<sizeof name){size_t z=strlen(cur(c)->text);if(n+z+3>=sizeof name)break;memcpy(name+n,cur(c)->text,z);n+=z;c->at++;if(!accept(c,"::"))break;name[n++]=':';name[n++]=':';}name[n]=0;
        if(accept(c,"(")){if(!strcmp(name,"__asm__")||!strcmp(name,"__c__")){if(!c->unsafe_depth)err(c,"inline assembly and C embedding require an unsafe block");if(cur(c)->kind!=TOK_STRING)err(c,"inline assembly and C embedding take one string literal");else c->at++;if(!accept(c,")"))err(c,"expected ')' after unsafe code literal");return "void";}Function*f=resolved(c,name);if(!f)err(c,"call to unknown function");return call_arguments(c,f);}
        const char*t=lookup(c,name);if(!t&&!strstr(name,"::")){for(ProjectUnit*u=c->project->head;u;u=u->next)for(Stmt*g=u->program.globals;g;g=g->next){const char*sep=strstr(g->name,"::");if(sep&&strlen(name)==strlen(sep+2)&&!strcmp(sep+2,name)){t=g->type?g->type:"int32_t";break;}}}
        if(t&&accept(c,".")){char member[256];if(cur(c)->kind!=TOK_ID){err(c,"expected field or method name after '.'");return "int32_t";}snprintf(member,sizeof member,"%s",cur(c)->text);c->at++;if(accept(c,"(")){Function*f=find_method(c->project,t,member);if(!f)err(c,"unknown method on object type");return call_arguments(c,f);}Field*f=find_field(c->project,t,member);if(!f){err(c,"unknown field on object type");return "int32_t";}return f->type;}
        if(t)return t;
        if(!strstr(name,"::")){/* namespace global lookup */for(ProjectUnit*u=c->project->head;u;u=u->next)for(Stmt*g=u->program.globals;g;g=g->next){const char*sep=strstr(g->name,"::");if(sep&&strlen(name)==strlen(sep+2)&&!strcmp(sep+2,name))return g->type?g->type:"int32_t";}}
        err(c,"use of undeclared identifier");return "int32_t";
    }
    err(c,"expected value in expression");if(cur(c)->kind!=TOK_EOF)c->at++;return "int32_t";
}
static const char *expression(Check*c,int minp){const char*left=primary(c);while(cur(c)->kind!=TOK_EOF){if(accept(c,"++")||accept(c,"--")){if(!numeric(left))err(c,"increment and decrement require a numeric operand");continue;}int p=precedence(cur(c)->text);if(p<minp||!p)break;Token op=*cur(c);c->at++;const char*right=expression(c,p+(!strcmp(op.text,"=")||!strcmp(op.text,"+=")||!strcmp(op.text,"-=" )?0:1));if(!strcmp(op.text,"=")||!strcmp(op.text,"+=")||!strcmp(op.text,"-=")){if(!compatible(left,right)&&!(c->unsafe_depth&&((pointer(left)&&numeric(right))||(pointer(right)&&numeric(left)))))err(c,"assignment types are incompatible");left=right;}else if(!strcmp(op.text,"&&")||!strcmp(op.text,"||")){if(!scalar(left)||!scalar(right))err(c,"logical operators require scalar operands");left="bool";}else if(p==4){if((strcmp(op.text,"==")&&strcmp(op.text,"!="))&&!(numeric(left)&&numeric(right)))err(c,"ordered comparison requires numeric operands");else if(!compatible(left,right)&&!compatible(right,left))err(c,"comparison operands have incompatible types");left="bool";}else {if(!numeric(left)||!numeric(right))err(c,"arithmetic operators require numeric operands");left=(!strcmp(left,"double")||!strcmp(right,"double"))?"double":(!strcmp(left,"float")||!strcmp(right,"float"))?"float":"int32_t";}}
    return left;}
static const char *check_expr(Check*c,Expr*e){if(!e->text||!*e->text)return "void";lexer_tokenize(e->text,c->unit->path,&c->tokens);c->at=0;const char*t=expression(c,1);if(cur(c)->kind!=TOK_EOF)err(c,"unexpected token in expression");e->inferred_type=copy_type(t);lexer_free(&c->tokens);return t;}
static void check_stmts(Check*c,Stmt*s){
    Symbol*base=c->symbols;
    for(;s;s=s->next)switch(s->kind){
    case ST_VAR:{
        const char*t=s->expr.text&&*s->expr.text?check_expr(c,&s->expr):s->type;
        if(s->infer_type){s->type=copy_type(t);s->infer_type=0;}
        if(s->type&&!compatible(s->type,t)&&!(c->unsafe_depth&&pointer(s->type)&&numeric(t)))err(c,"initializer type does not match variable type");
        push(c,s->name,s->type?s->type:t);break;
    }
    case ST_PRINT:check_expr(c,&s->expr);break;
    case ST_EXPR:check_expr(c,&s->expr);break;
    case ST_RETURN:{const char*t=s->expr.text&&*s->expr.text?check_expr(c,&s->expr):"void";if(c->return_type&&!compatible(c->return_type,t))err(c,"return value type does not match function return type");break;}
    case ST_IF:{
        const char*t=check_expr(c,&s->cond);if(!scalar(t))err(c,"condition must have a scalar type");
        Symbol*mark=c->symbols;check_stmts(c,s->body);pop_to(c,mark);
        mark=c->symbols;check_stmts(c,s->otherwise);pop_to(c,mark);break;
    }
    case ST_WHILE:{
        const char*t=check_expr(c,&s->cond);if(!scalar(t))err(c,"condition must have a scalar type");
        Symbol*mark=c->symbols;c->loop_depth++;check_stmts(c,s->body);c->loop_depth--;pop_to(c,mark);break;
    }
    case ST_BREAK:if(!c->loop_depth)err(c,"break is only valid inside a loop");break;
    case ST_CONTINUE:if(!c->loop_depth)err(c,"continue is only valid inside a loop");break;
    case ST_FOR:{
        Symbol*mark=c->symbols;Stmt*init=s->initializer;
        if(init&&init->kind==ST_VAR){
            const char*t=init->expr.text&&*init->expr.text?check_expr(c,&init->expr):init->type;
            if(init->infer_type){init->type=copy_type(t);init->infer_type=0;}
            if(init->type&&!compatible(init->type,t)&&!(c->unsafe_depth&&pointer(init->type)&&numeric(t)))err(c,"for initializer type does not match variable type");
            push(c,init->name,init->type?init->type:t);
        }else if(init&&init->kind==ST_EXPR)check_expr(c,&init->expr);
        if(s->cond.text&&*s->cond.text){const char*t=check_expr(c,&s->cond);if(!scalar(t))err(c,"for condition must have a scalar type");}
        if(s->step.text&&*s->step.text)check_expr(c,&s->step);
        c->loop_depth++;check_stmts(c,s->body);c->loop_depth--;pop_to(c,mark);break;
    }
    case ST_BLOCK:{Symbol*mark=c->symbols;if(s->is_unsafe)c->unsafe_depth++;check_stmts(c,s->body);if(s->is_unsafe)c->unsafe_depth--;pop_to(c,mark);break;}
    }
    pop_to(c,base);
}
static void check_function(Check*c,Function*f){Symbol*mark=c->symbols;c->return_type=f->return_type;c->owner=f->owner;c->name_space=NULL;const char*fullname=f->is_method?f->owner:f->name;const char*sep=fullname?strrchr(fullname,':'):NULL;if(sep&&sep>fullname&&sep[-1]==':'){size_t n=(size_t)(sep-fullname-1);if(n<sizeof c->namespace_storage){memcpy(c->namespace_storage,fullname,n);c->namespace_storage[n]=0;c->name_space=c->namespace_storage;}}for(Param*p=f->params;p;p=p->next)push(c,p->name,p->type);if(f->is_method){ClassDecl*cl=find_class(c->project,f->owner);for(;cl;cl=cl->base_name?find_class(c->project,cl->base_name):NULL)for(Field*x=cl->fields;x;x=x->next)push(c,x->name,x->type);}if(!f->is_external&&!f->is_interface_method)check_stmts(c,f->body);pop_to(c,mark);}
int typecheck_project(Project*project){int ok=project->errors==0;for(ProjectUnit*u=project->head;u;u=u->next)for(Stmt*g=u->program.globals;g;g=g->next)if(g->infer_type){Check c={0};c.project=project;c.unit=u;g->type=copy_type(check_expr(&c,&g->expr));}
    for(ProjectUnit*u=project->head;u;u=u->next){Check c={0};c.project=project;c.unit=u;for(Stmt*g=u->program.globals;g;g=g->next)push(&c,g->name,g->type?g->type:"int32_t");for(ProjectUnit*v=project->head;v;v=v->next)for(Stmt*g=v->program.globals;g;g=g->next)if(!strstr(g->name,"::"))push(&c,g->name,g->type?g->type:"int32_t");check_stmts(&c,u->program.globals);check_stmts(&c,u->program.module_stmts);for(Function*f=u->program.functions;f;f=f->next)check_function(&c,f);for(ClassDecl*k=u->program.classes;k;k=k->next)for(Function*f=k->methods;f;f=f->next)check_function(&c,f);while(c.symbols){Symbol*s=c.symbols;c.symbols=s->next;free(s);}if(c.errors)ok=0;}return ok;}
