#include <mobdrive/parser.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { const TokenList *t; size_t at; int errors; char *name_space; } P;
static Token *cur(P *p){return &p->t->items[p->at];}
static int is(P *p,const char *s){return strcmp(cur(p)->text,s)==0;}
static Token take(P *p){return p->t->items[p->at++];}
static int accept(P *p,const char *s){if(is(p,s)){p->at++;return 1;}return 0;}
static void error(P *p,const char *m){fprintf(stderr,"%s:%u: error: %s (got '%s')\n",p->t->filename,cur(p)->line,m,cur(p)->text);p->errors++;}
static int expect(P *p,const char *s){if(accept(p,s))return 1;char b[128];snprintf(b,sizeof b,"expected '%s'",s);error(p,b);return 0;}
static char *dup(const char *s){size_t n=strlen(s)+1;char *r=(char*)malloc(n);if(r)memcpy(r,s,n);return r;}
static char *identifier(P *p);
static char *qualify(P*p,const char*name){if(!p->name_space||!*p->name_space)return dup(name);size_t a=strlen(p->name_space),b=strlen(name);char*r=(char*)malloc(a+b+3);memcpy(r,p->name_space,a);memcpy(r+a,"::",2);memcpy(r+a+2,name,b+1);return r;}
static char *qualified_identifier(P*p){char*name=identifier(p);size_t len=strlen(name);while(accept(p,"::")){char*part=identifier(p);char*r=(char*)malloc(len+strlen(part)+3);memcpy(r,name,len);memcpy(r+len,"::",2);strcpy(r+len+2,part);len+=strlen(part)+2;free(name);free(part);name=r;}return name;}
static char *qualified_ref(P*p){char*name=qualified_identifier(p);if(strstr(name,"::")){if(p->name_space&&strncmp(name,p->name_space,strlen(p->name_space))==0&&name[strlen(p->name_space)]==':')return name;return name;}char*r=qualify(p,name);free(name);return r;}
static int is_type(P *p){
    static const char *types[]={"void","int","int8","int16","int32","int64","uint8","uint16","uint32","uint64","i8","i16","i32","i64","u8","u16","u32","u64","float","double","bool","char","string"};
    for(size_t i=0;i<sizeof(types)/sizeof(types[0]);i++)if(is(p,types[i]))return 1;
    return 0;
}
static char *type(P *p){
    static const char *types[]={"void","int","int8","int16","int32","int64","uint8","uint16","uint32","uint64","i8","i16","i32","i64","u8","u16","u32","u64","float","double","bool","char","string"};
    unsigned pointers=0;while(accept(p,"*"))pointers++;
    for(size_t i=0;i<sizeof(types)/sizeof(types[0]);i++)if(accept(p,types[i])){
        const char *mapped=types[i];
        if(!strcmp(mapped,"i8")||!strcmp(mapped,"int8"))mapped="int8_t";else if(!strcmp(mapped,"i16")||!strcmp(mapped,"int16"))mapped="int16_t";else if(!strcmp(mapped,"i32")||!strcmp(mapped,"int32"))mapped="int32_t";else if(!strcmp(mapped,"i64")||!strcmp(mapped,"int64"))mapped="int64_t";
        else if(!strcmp(mapped,"u8")||!strcmp(mapped,"uint8"))mapped="uint8_t";else if(!strcmp(mapped,"u16")||!strcmp(mapped,"uint16"))mapped="uint16_t";else if(!strcmp(mapped,"u32")||!strcmp(mapped,"uint32"))mapped="uint32_t";else if(!strcmp(mapped,"u64")||!strcmp(mapped,"uint64"))mapped="uint64_t";else if(!strcmp(mapped,"string"))mapped="const char *";
        while(accept(p,"*"))pointers++;
        size_t n=strlen(mapped);char*out=(char*)malloc(n+(size_t)pointers*2+1);memcpy(out,mapped,n);
        for(unsigned j=0;j<pointers;j++){memcpy(out+n," *",2);n+=2;}out[n]=0;
        return out;
    }
    if(cur(p)->kind==TOK_ID)return qualified_ref(p);
    error(p,"expected a supported type");if(cur(p)->kind!=TOK_EOF)p->at++;return dup("int");
}
static char *identifier(P *p){if(cur(p)->kind!=TOK_ID){error(p,"expected identifier");if(cur(p)->kind!=TOK_EOF)p->at++;return dup("_error");}return dup(take(p).text);}
static void append(char **s,size_t *len,size_t *cap,const char *v,int space){size_t n=strlen(v),need=*len+n+(space?1:0)+1;if(need>*cap){*cap=need*2;*s=(char*)realloc(*s,*cap);}if(space&&*len)(*s)[(*len)++]=' ';memcpy(*s+*len,v,n);*len+=n;(*s)[*len]=0;}
static Expr expr(P *p,const char *end1,const char *end2){
    Expr e={dup(""),NULL};size_t len=0,cap=1;int par=0,bracket=0,any=0;
    while(cur(p)->kind!=TOK_EOF){if(!par&&!bracket&&((end1&&is(p,end1))||(end2&&is(p,end2))))break;
        Token t=take(p);if(!strcmp(t.text,"("))par++;else if(!strcmp(t.text,")"))par--;else if(!strcmp(t.text,"["))bracket++;else if(!strcmp(t.text,"]"))bracket--;
        append(&e.text,&len,&cap,t.text,t.kind==TOK_ID||t.kind==TOK_NUMBER||t.kind==TOK_STRING);any=1;
    }
    if(!any)error(p,"expected expression");
    return e;
}
static Stmt *statement(P *p);
static Stmt *block(P *p){expect(p,"{");Stmt *head=NULL,**tail=&head;while(cur(p)->kind!=TOK_EOF&&!is(p,"}")){size_t before=p->at;Stmt *s=statement(p);if(s){*tail=s;while(*tail)tail=&(*tail)->next;}if(before==p->at){error(p,"unable to parse statement");p->at++;}}expect(p,"}");accept(p,";");return head;}
static Stmt *statement(P *p){
    Stmt *s=(Stmt*)calloc(1,sizeof(*s));if(!s)return NULL;
    if(accept(p,"safe")||accept(p,"unsafe")){s->kind=ST_BLOCK;s->is_unsafe=!strcmp(p->t->items[p->at-1].text,"unsafe");s->body=block(p);return s;}
    if(accept(p,"let")||accept(p,"var")||accept(p,"const")){
        const char *decl=p->t->items[p->at-1].text;
        s->kind=ST_VAR;s->is_const=!strcmp(decl,"const");
        if(is_type(p)){s->type=type(p);s->name=identifier(p);}
        else {s->name=identifier(p);if(accept(p,":"))s->type=type(p);else if(!s->is_const)s->infer_type=1;else error(p,"const declarations require an explicit type");}
        if(accept(p,"="))s->expr=expr(p,";",NULL);
        else if(s->is_const)error(p,"const declarations require an initializer");
        else if(s->infer_type)error(p,"type inference requires an initializer");
        expect(p,";");return s;
    }
    if(accept(p,"print")){s->kind=ST_PRINT;expect(p,"(");s->expr=expr(p,")",NULL);expect(p,")");expect(p,";");return s;}
    if(accept(p,"return")){s->kind=ST_RETURN;if(!is(p,";"))s->expr=expr(p,";",NULL);expect(p,";");return s;}
    if(accept(p,"if")){s->kind=ST_IF;expect(p,"(");s->cond=expr(p,")",NULL);expect(p,")");s->body=block(p);if(accept(p,"else"))s->otherwise=block(p);return s;}
    if(accept(p,"while")){s->kind=ST_WHILE;expect(p,"(");s->cond=expr(p,")",NULL);expect(p,")");s->body=block(p);return s;}
    if(is(p,"}")){free(s);return NULL;}
    s->kind=ST_EXPR;s->expr=expr(p,";",NULL);expect(p,";");return s;
}
static Function *function_signature_mode(P *p,int external,int method,int interface_method,const char *owner,int has_function_keyword){
    if(has_function_keyword)expect(p,"function");
    Function *f=(Function*)calloc(1,sizeof(*f));
    f->is_external=external;f->is_method=method;f->is_interface_method=interface_method;
    f->owner=owner?dup(owner):NULL;f->name=identifier(p);if(!method&&!interface_method){char*q=qualify(p,f->name);free(f->name);f->name=q;}expect(p,"(");Param **tail=&f->params;
    while(cur(p)->kind!=TOK_EOF&&!is(p,")")){
        Param *a=(Param*)calloc(1,sizeof(*a));
        if(is_type(p)){a->type=type(p);a->name=identifier(p);}
        else {a->name=identifier(p);expect(p,":");a->type=type(p);}
        *tail=a;tail=&a->next;if(!accept(p,","))break;
    }
    expect(p,")");
    if(external&&accept(p,"::")){
        if(accept(p,"link"))f->link_mode=dup("link");else if(accept(p,"nolink"))f->link_mode=dup("nolink");else if(accept(p,"decide"))f->link_mode=dup("decide");else error(p,"expected link, nolink, or decide");
        expect(p,"(");expect(p,")");
    }
    expect(p,"->");f->return_type=type(p);
    if(external||interface_method)expect(p,";");else f->body=block(p);
    return f;
}
static Function *function_signature(P *p,int external,int method,int interface_method,const char *owner){return function_signature_mode(p,external,method,interface_method,owner,1);}
static void function(P *p,Program *pr,int external){
    Function *f=function_signature(p,external,0,0,NULL);
    if(!pr->functions_tail)pr->functions_tail=&pr->functions;
    *pr->functions_tail=f;pr->functions_tail=&f->next;
}
static void append_name(NameRef **tail,P*p){NameRef*n=(NameRef*)calloc(1,sizeof(*n));n->name=qualified_ref(p);*tail=n;}
static void class_decl(P*p,Program*pr){
    int graphic=accept(p,"graphic");expect(p,"class");ClassDecl*c=(ClassDecl*)calloc(1,sizeof(*c));c->is_graphic=graphic;char*local=identifier(p);c->name=qualify(p,local);free(local);
    if(accept(p,"extends"))c->base_name=qualified_ref(p);
    if(accept(p,"implements")||accept(p,"plug")){NameRef**tail=&c->interfaces;append_name(tail,p);tail=&(*tail)->next;while(accept(p,",")||accept(p,"plug")){append_name(tail,p);tail=&(*tail)->next;}}
    expect(p,"{");Field**fields=&c->fields;Function**methods=&c->methods;
    while(cur(p)->kind!=TOK_EOF&&!is(p,"}")){
        while(accept(p,"public")||accept(p,"private")||accept(p,"protected")){}
        int visual=accept(p,"visual");
        if(accept(p,"var")||accept(p,"let")||accept(p,"const")){
            int cn=!strcmp(p->t->items[p->at-1].text,"const");Field*f=(Field*)calloc(1,sizeof(*f));f->is_const=cn;f->name=identifier(p);
            if(accept(p,":"))f->type=type(p);else if(is_type(p)){f->type=type(p);free(f->name);f->name=identifier(p);}else error(p,"class fields require a type");
            if(accept(p,"=")){error(p,"field initializers are not supported yet");Expr ignored=expr(p,";",NULL);free(ignored.text);}expect(p,";");*fields=f;fields=&f->next;
        }else if(is(p,"function")||visual){Function*f=function_signature_mode(p,0,1,0,c->name,!visual);f->is_visual=visual;*methods=f;methods=&f->next;}
        else {error(p,"expected a field or method declaration in class");p->at++;}
    }
    expect(p,"}");accept(p,";");
    if(!pr->classes_tail)pr->classes_tail=&pr->classes;
    *pr->classes_tail=c;pr->classes_tail=&c->next;
}
static void interface_decl(P*p,Program*pr){
    if(accept(p,"socket")){}else expect(p,"interface");InterfaceDecl*i=(InterfaceDecl*)calloc(1,sizeof(*i));char*local=identifier(p);i->name=qualify(p,local);free(local);expect(p,"{");Function**tail=&i->methods;
    while(cur(p)->kind!=TOK_EOF&&!is(p,"}")){
        if(!accept(p,"public")&&!accept(p,"private"))accept(p,"protected");
        Function*f=function_signature(p,0,0,1,i->name);*tail=f;tail=&f->next;
    }
    expect(p,"}");accept(p,";");
    if(!pr->interfaces_tail)pr->interfaces_tail=&pr->interfaces;
    *pr->interfaces_tail=i;pr->interfaces_tail=&i->next;
}
static char *path_token(P *p){
    if(cur(p)->kind==TOK_STRING){char *q=take(p).text;size_t n=strlen(q);if(n>=2){q[n-1]=0;memmove(q,q+1,n-1);}return q;}
    char *r=dup("");size_t len=0,cap=1;
    while(cur(p)->kind!=TOK_EOF&&!is(p,")")){Token t=take(p);append(&r,&len,&cap,t.text,0);}return r;
}
static void import_decl(P *p,Program *pr){
    expect(p,"enter");Import *im=(Import*)calloc(1,sizeof(*im));expect(p,"(");im->source_path=path_token(p);expect(p,")");
    if(accept(p,"{")){while(cur(p)->kind!=TOK_EOF&&!is(p,"}")){
        expect(p,"const");expect(p,"path");Token t=take(p);if(t.kind==TOK_STRING){size_t n=strlen(t.text);if(n>=2){t.text[n-1]=0;memmove(t.text,t.text+1,n-1);}im->artifact_path=dup(t.text);}else {error(p,"path requires a quoted artifact filename");}
        expect(p,";");
    }expect(p,"}");accept(p,";");}else expect(p,";");
    if(!pr->imports_tail)pr->imports_tail=&pr->imports;
    *pr->imports_tail=im;pr->imports_tail=&im->next;
}
static void check_decl(P *p,Program *pr){
    expect(p,"nochk");CheckRule *r=(CheckRule*)calloc(1,sizeof(*r));expect(p,"{");
    while(cur(p)->kind!=TOK_EOF&&!is(p,"}")){char *n=qualified_ref(p);r->names=(char**)realloc(r->names,(r->count+1)*sizeof(char*));r->names[r->count++]=n;if(!accept(p,","))break;}expect(p,"}");expect(p,"::");
    if(accept(p,"none"))r->level=dup("none");else if(accept(p,"minchk"))r->level=dup("minchk");else if(accept(p,"strict"))r->level=dup("strict");else{error(p,"expected none, minchk, or strict");r->level=dup("strict");}expect(p,";");
    if(!pr->checks_tail)pr->checks_tail=&pr->checks;
    *pr->checks_tail=r;pr->checks_tail=&r->next;
}
static void top_item(P*p,Program*pr);
static void namespace_decl(P*p,Program*pr){
    expect(p,"namespace");char*local=identifier(p);char*full=qualify(p,local);free(local);
    char*previous=p->name_space;p->name_space=full;expect(p,"{");
    while(cur(p)->kind!=TOK_EOF&&!is(p,"}")){size_t before=p->at;top_item(p,pr);if(before==p->at){error(p,"unable to parse namespace declaration");p->at++;}}
    expect(p,"}");accept(p,";");free(p->name_space);p->name_space=previous;
}
static void top_item(P*p,Program*pr){
    if(accept(p,"enter")){p->at--;import_decl(p,pr);}
    else if(accept(p,"nochk")){p->at--;check_decl(p,pr);}
    else if(is(p,"namespace"))namespace_decl(p,pr);
    else if(is(p,"class")||(is(p,"graphic")&&p->at+1<p->t->count&&!strcmp(p->t->items[p->at+1].text,"class"))){class_decl(p,pr);}
    else if(is(p,"interface")||is(p,"socket")){interface_decl(p,pr);}
    else if(is(p,"const")||is(p,"var")||is(p,"let")){Stmt*g=statement(p);char*q=qualify(p,g->name);free(g->name);g->name=q;if(!pr->globals_tail)pr->globals_tail=&pr->globals;*pr->globals_tail=g;pr->globals_tail=&g->next;}
    else if(is(p,"safe")||is(p,"unsafe")){Stmt*s=statement(p);if(!pr->module_stmts_tail)pr->module_stmts_tail=&pr->module_stmts;*pr->module_stmts_tail=s;pr->module_stmts_tail=&s->next;}
    else {if(!accept(p,"public")&&!accept(p,"private"))accept(p,"protected");int ext=accept(p,"bin");if(accept(p,"function")){p->at--;function(p,pr,ext);}else{error(p,"expected function, class, interface, namespace, variable, safe block, or unsafe block");if(cur(p)->kind!=TOK_EOF)p->at++;}}
}
int parser_parse(const TokenList *tokens,Program *pr){P p={tokens,0,tokens->errors,NULL};memset(pr,0,sizeof(*pr));
    while(cur(&p)->kind!=TOK_EOF){size_t before=p.at;top_item(&p,pr);if(before==p.at){error(&p,"unable to parse declaration");p.at++;}}
    free(p.name_space);return p.errors==0;
}
static void free_stmts(Stmt *s){while(s){Stmt *n=s->next;free(s->name);free(s->type);free(s->expr.text);free(s->expr.inferred_type);free(s->cond.text);free(s->cond.inferred_type);free_stmts(s->body);free_stmts(s->otherwise);free(s);s=n;}}
static void free_function(Function*f){while(f){Function*n=f->next;Param*a=f->params;while(a){Param*x=a->next;free(a->name);free(a->type);free(a);a=x;}free(f->name);free(f->return_type);free(f->link_mode);free(f->owner);free_stmts(f->body);free(f);f=n;}}
void parser_free(Program *p){
    free_stmts(p->globals);
    free_stmts(p->module_stmts);
    free_function(p->functions);
    ClassDecl*k=p->classes;while(k){ClassDecl*n=k->next;free(k->name);free(k->base_name);Field*f=k->fields;while(f){Field*x=f->next;free(f->name);free(f->type);free(f);f=x;}free_function(k->methods);NameRef*r=k->interfaces;while(r){NameRef*x=r->next;free(r->name);free(r);r=x;}free(k);k=n;}
    InterfaceDecl*iface=p->interfaces;while(iface){InterfaceDecl*n=iface->next;free(iface->name);free_function(iface->methods);free(iface);iface=n;}
    Import*im=p->imports;while(im){Import*n=im->next;free(im->source_path);free(im->artifact_path);free(im);im=n;}
    CheckRule*c=p->checks;while(c){CheckRule*n=c->next;for(size_t x=0;x<c->count;x++)free(c->names[x]);free(c->names);free(c->level);free(c);c=n;}
    memset(p,0,sizeof(*p));
}
