#include <mobdrive/codegen.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ClassDecl *find_class(const Project*p,const char*name){for(ProjectUnit*u=p->head;u;u=u->next)for(ClassDecl*c=u->program.classes;c;c=c->next)if(!strcmp(c->name,name))return c;return NULL;}
static void indent(FILE*f,int n){while(n--)fputs("    ",f);}
static void emit_c_name(FILE*f,const char*name){for(const char*s=name;*s;s++){if(s[0]==':'&&s[1]==':'){fputs("__",f);s++;}else fputc(*s,f);}}
static void emit_type(FILE*f,const Project*p,const char*type){if(!type){fputs("int32_t",f);return;}const char*star=strchr(type,'*');size_t n=star?(size_t)(star-type):strlen(type);while(n&&isspace((unsigned char)type[n-1]))n--;char name[512];if(n>=sizeof name)n=sizeof name-1;memcpy(name,type,n);name[n]=0;if(find_class(p,name))emit_c_name(f,name);else fputs(name,f);if(star)fputs(star,f);}
static int statement_declares(const Stmt*s,const char*name){
    for(;s;s=s->next){if(s->kind==ST_VAR&&s->name&&!strcmp(s->name,name))return 1;if(statement_declares(s->body,name)||statement_declares(s->otherwise,name))return 1;}
    return 0;
}
static int local_name(const Function*fn,const char*name){
    for(Param*p=fn?fn->params:NULL;p;p=p->next)if(!strcmp(p->name,name))return 1;
    return statement_declares(fn?fn->body:NULL,name);
}
static const char *local_type(const Function*fn,const char*name){for(Param*p=fn?fn->params:NULL;p;p=p->next)if(!strcmp(p->name,name))return p->type;for(const Stmt*s=fn?fn->body:NULL;s;s=s->next){if(s->kind==ST_VAR&&!strcmp(s->name,name))return s->type?s->type:"int32_t";for(const Stmt*b=s->body;b;b=b->next)if(b->kind==ST_VAR&&!strcmp(b->name,name))return b->type?b->type:"int32_t";}return NULL;}
static const char *qualified_symbol(const Project*p,const Function*fn,const char*name,int call){
    const char*scope=fn?(fn->is_method?fn->owner:fn->name):NULL;const char*sep=scope?strrchr(scope,':'):NULL;
    if(sep&&sep>scope&&sep[-1]==':'){
        size_t prefix=(size_t)(sep-scope-1),n=strlen(name);char*q=(char*)malloc(prefix+n+3);if(q){memcpy(q,scope,prefix);memcpy(q+prefix,"::",2);memcpy(q+prefix+2,name,n+1);
            if(call){for(ProjectUnit*u=p->head;u;u=u->next)for(Function*f=u->program.functions;f;f=f->next)if(!strcmp(f->name,q)){free(q);return f->name;}}
            else {for(ProjectUnit*u=p->head;u;u=u->next)for(Stmt*g=u->program.globals;g;g=g->next)if(!strcmp(g->name,q)){free(q);return g->name;}}
            free(q);
        }
    }
    for(ProjectUnit*u=p->head;u;u=u->next){if(call){for(Function*f=u->program.functions;f;f=f->next)if(!strcmp(f->name,name))return f->name;}else for(Stmt*g=u->program.globals;g;g=g->next)if(!strcmp(g->name,name))return g->name;}
    return NULL;
}
static int field_access(const Project*p,const ClassDecl*c,const char*name,FILE*f){
    for(const ClassDecl*k=c;k;k=k->base_name?find_class(p,k->base_name):NULL){
        for(Field*x=k->fields;x;x=x->next)if(!strcmp(x->name,name)){
            fputs("self->",f);for(const ClassDecl*walk=c;walk&&walk!=k;walk=walk->base_name?find_class(p,walk->base_name):NULL)fputs("base.",f);fputs(name,f);return 1;
        }
    }
    return 0;
}
static int emit_c_embed(FILE*f,const char**source){const char*s=*source;while(isspace((unsigned char)*s))s++;if(*s!='(')return 0;s++;while(isspace((unsigned char)*s))s++;if(*s!='"')return 0;s++;while(*s&&*s!='"'){char c=*s++;if(c=='\\'&&*s){char e=*s++;switch(e){case 'n':c='\n';break;case 'r':c='\r';break;case 't':c='\t';break;default:c=e;break;}}fputc(c,f);}if(*s!='"')return 0;s++;while(isspace((unsigned char)*s))s++;if(*s!=')')return 0;*source=s+1;return 1;}
static void emit_expression(FILE*f,const char*text,const Project*p,const ClassDecl*owner,const Function*fn){
    const char*s=text;
    while(*s){
        if(*s=='"'||*s=='\''){char q=*s++;fputc(q,f);while(*s){char c=*s++;fputc(c,f);if(c=='\\'&&*s)fputc(*s++,f);else if(c==q)break;}continue;}
        if(isalpha((unsigned char)*s)||*s=='_'){
            char id[256];size_t n=0;while((isalnum((unsigned char)*s)||*s=='_')&&n+1<sizeof id)id[n++]=*s++;id[n]=0;
            const char*look=s;while(isspace((unsigned char)*look))look++;
            if(!strcmp(id,"__c__")){s=look;if(emit_c_embed(f,&s))continue;fputs("__c__",f);continue;}
            if(!strcmp(id,"new")){s=look;char type_name[512];size_t tn=0;while((isalnum((unsigned char)*s)||*s=='_'||(s[0]==':'&&s[1]==':'))&&tn+3<sizeof type_name){if(s[0]==':'&&s[1]==':'){type_name[tn++]=':';type_name[tn++]=':';s+=2;}else type_name[tn++]=*s++;}type_name[tn]=0;while(isspace((unsigned char)*s))s++;if(*s=='('){s++;while(isspace((unsigned char)*s))s++;if(*s==')'){s++;fputc('(',f);emit_type(f,p,type_name);fputs("){0}",f);continue;}}fputs("new ",f);emit_c_name(f,type_name);continue;}
            if(*look=='.'){const char*type=local_type(fn,id);if(type&&find_class(p,type)){const char*m=look+1;while(isspace((unsigned char)*m))m++;char method[256];size_t mn=0;while((isalnum((unsigned char)*m)||*m=='_')&&mn+1<sizeof method)method[mn++]=*m++;method[mn]=0;while(isspace((unsigned char)*m))m++;if(*m=='('){emit_c_name(f,type);fputc('_',f);emit_c_name(f,method);fputs("(&",f);fputs(id,f);m++;while(isspace((unsigned char)*m))m++;if(*m==')'){fputc(')',f);s=m+1;continue;}fputs(", ",f);s=m;continue;}}}
            if(look[0]==':'&&look[1]==':'){
                char qualified[1024];snprintf(qualified,sizeof qualified,"%s",id);size_t qn=strlen(qualified);s=look;
                while(s[0]==':'&&s[1]==':'){s+=2;while(isspace((unsigned char)*s))s++;size_t m=0;char part[256];while((isalnum((unsigned char)*s)||*s=='_')&&m+1<sizeof part)part[m++]=*s++;part[m]=0;if(qn+2+m+1<sizeof qualified){qualified[qn++]=':';qualified[qn++]=':';memcpy(qualified+qn,part,m+1);qn+=m;}}
                emit_c_name(f,qualified);continue;
            }
            if(!strcmp(id,"null")){fputs("NULL",f);continue;}
            if(strcmp(id,"true")&&strcmp(id,"false")&&strcmp(id,"self")&&!local_name(fn,id)&&field_access(p,owner,id,f))continue;
            if(!local_name(fn,id)){const char*symbol=qualified_symbol(p,fn,id,*look=='(');if(symbol){emit_c_name(f,symbol);continue;}}
            fputs(id,f);continue;
        }
        fputc(*s++,f);
    }
}
static const char *infer_type(const char*expr){
    if(!expr)return "int32_t";
    while(isspace((unsigned char)*expr))expr++;
    if(*expr=='"'||*expr=='\'')return *expr=='"'?"const char *":"char";
    if(!strcmp(expr,"true")||!strcmp(expr,"false"))return "bool";
    for(const char*p=expr;*p;p++)if(*p=='.')return "double";
    return "int32_t";
}
static void emit_statements(FILE*f,const Stmt*s,int depth,const Project*p,const ClassDecl*owner,const Function*fn){
    for(;s;s=s->next){indent(f,depth);switch(s->kind){
    case ST_VAR:{const char*ty=s->type?s->type:infer_type(s->expr.text);if(s->is_const&&!strcmp(ty,"const char *")){fputs("const char * const ",f);emit_c_name(f,s->name);}else{if(s->is_const)fputs("const ",f);emit_type(f,p,ty);fputc(' ',f);emit_c_name(f,s->name);}if(s->expr.text){fputs(" = ",f);const char*e=s->expr.text;while(isspace((unsigned char)*e))e++;if(strchr(ty,'*')&&isdigit((unsigned char)*e)){fputc('(',f);emit_type(f,p,ty);fputs(")(uintptr_t)(",f);emit_expression(f,e,p,owner,fn);fputc(')',f);}else emit_expression(f,e,p,owner,fn);}fputs(";\n",f);break;}
    case ST_PRINT:fputs("mbd_print(",f);emit_expression(f,s->expr.text,p,owner,fn);fputs(");\n",f);break;
    case ST_RETURN:fputs("return",f);if(s->expr.text){fputc(' ',f);emit_expression(f,s->expr.text,p,owner,fn);}fputs(";\n",f);break;
    case ST_EXPR:emit_expression(f,s->expr.text,p,owner,fn);fputs(";\n",f);break;
    case ST_BLOCK:fputs("{\n",f);emit_statements(f,s->body,depth+1,p,owner,fn);indent(f,depth);fputs("}\n",f);break;
    case ST_IF:fputs("if (",f);emit_expression(f,s->cond.text,p,owner,fn);fputs(") {\n",f);emit_statements(f,s->body,depth+1,p,owner,fn);indent(f,depth);fputs("}",f);if(s->otherwise){fputs(" else {\n",f);emit_statements(f,s->otherwise,depth+1,p,owner,fn);indent(f,depth);fputs("}",f);}fputc('\n',f);break;
    case ST_WHILE:fputs("while (",f);emit_expression(f,s->cond.text,p,owner,fn);fputs(") {\n",f);emit_statements(f,s->body,depth+1,p,owner,fn);indent(f,depth);fputs("}\n",f);break;
    }}
}
static void emit_params(FILE*f,const Project*p,const Function*fn){
    int first=1;
    if(fn->is_method){emit_c_name(f,fn->owner);fputs(" *self",f);first=0;}
    for(Param*a=fn->params;a;a=a->next){if(!first)fputs(", ",f);emit_type(f,p,a->type);fprintf(f," %s",a->name);first=0;}
    if(first)fputs("void",f);
}
static void emit_signature(FILE*f,const Project*p,const Function*fn,int prototype){
    if(fn->is_external)fputs("extern ",f);
    emit_type(f,p,fn->return_type);fputc(' ',f);emit_c_name(f,fn->name);fputc('(',f);
    emit_params(f,p,fn);fputc(')',f);
    if(prototype)fputs(";\n",f);
    else {fputs(" {\n",f);ClassDecl*owner=fn->is_method?NULL:NULL;(void)owner;fputs("}\n",f);}
}
static void emit_forward_types(FILE*f,const Project*p){for(ProjectUnit*u=p->head;u;u=u->next)for(ClassDecl*c=u->program.classes;c;c=c->next){fputs("typedef struct ",f);emit_c_name(f,c->name);fputc(' ',f);emit_c_name(f,c->name);fputs(";\n",f);}fputc('\n',f);}
static void emit_class_struct(FILE*f,const Project*p,const ClassDecl*c,ClassDecl**done,size_t*count){
    for(size_t i=0;i<*count;i++)if(done[i]==c)return;
    if(c->base_name){ClassDecl*b=find_class(p,c->base_name);if(b)emit_class_struct(f,p,b,done,count);}
    done[(*count)++]=(ClassDecl*)c;
    fputs("struct ",f);emit_c_name(f,c->name);fputs(" {\n",f);if(c->base_name){fputs("    ",f);emit_c_name(f,c->base_name);fputs(" base;\n",f);}
    int has_member=c->base_name!=NULL;
    for(Field*x=c->fields;x;x=x->next){if(x->is_const&&!strcmp(x->type,"const char *"))fprintf(f,"    const char * const %s;\n",x->name);else{fputs("    ",f);if(x->is_const)fputs("const ",f);emit_type(f,p,x->type);fprintf(f," %s;\n",x->name);}has_member=1;}
    if(!has_member)fputs("    unsigned char _mbd_empty;\n",f);
    fprintf(f,"};\n\n");
}
static void emit_class_definitions(FILE*f,const Project*p){
    size_t total=0;for(ProjectUnit*u=p->head;u;u=u->next)for(ClassDecl*c=u->program.classes;c;c=c->next)total++;
    ClassDecl**done=(ClassDecl**)calloc(total?total:1,sizeof(*done));size_t count=0;
    for(ProjectUnit*u=p->head;u;u=u->next)for(ClassDecl*c=u->program.classes;c;c=c->next)emit_class_struct(f,p,c,done,&count);
    free(done);
}
static void emit_method(FILE*f,const Project*p,const ClassDecl*c,const Function*fn,int prototype){
    if(!fn->is_method)return;
    emit_type(f,p,fn->return_type);fputc(' ',f);emit_c_name(f,c->name);fputc('_',f);emit_c_name(f,fn->name);fputc('(',f);
    emit_params(f,p,fn);fputc(')',f);
    if(prototype){fputs(";\n",f);return;}
    fputs(" {\n",f);emit_statements(f,fn->body,1,p,c,fn);
    if(c->is_graphic&&fn->is_visual&&!strcmp(fn->return_type,"void")){
        int has_title=0;for(Field*x=c->fields;x;x=x->next)if(!strcmp(x->name,"title"))has_title=1;
        fputs("    if (ui_window_open(",f);
        if(has_title)fputs("(self->title ? self->title : ",f);
        fputc('"',f);for(const char*s=c->name;*s;s++){if(*s=='"'||*s=='\\')fputc('\\',f);fputc(*s,f);}fputc('"',f);
        if(has_title)fputc(')',f);
        fputs(", 800, 600)) ui_event_loop();\n",f);
    }
    fputs("}\n",f);
}
static int codegen_emit_c_mode(const Project*project,const char*path,int freestanding){
    FILE*f=fopen(path,"wb");if(!f){fprintf(stderr,"mobdrive: cannot create '%s'\n",path);return 0;}
    if(freestanding){fputs("#include <stdbool.h>\n#include <stdint.h>\n#include <stddef.h>\n",f);fputs("extern int mbd_print_string(const char *);\nextern int mbd_print_float(double);\nextern int mbd_print_int(long long);\n",f);}
    else {fputs("#include <stdio.h>\n#include <stdbool.h>\n#include <stdint.h>\n",f);fputs("static int mbd_print_string(const char *x){return printf(\"%s\\n\",x);}\nstatic int mbd_print_float(double x){return printf(\"%g\\n\",x);}\nstatic int mbd_print_int(long long x){return printf(\"%lld\\n\",x);}\n",f);}
    fputs("#define mbd_print(x) _Generic((x), const char*: mbd_print_string, char*: mbd_print_string, float: mbd_print_float, double: mbd_print_float, default: mbd_print_int)(x)\n\n",f);
    emit_forward_types(f,project);emit_class_definitions(f,project);
    for(ProjectUnit*u=project->head;u;u=u->next)emit_statements(f,u->program.globals,0,project,NULL,NULL);
    fputc('\n',f);
    size_t module_index=0;
    for(ProjectUnit*u=project->head;u;u=u->next){if(u->program.module_stmts)fprintf(f,"static void __mbd_module_init_%zu(void);\n",module_index);module_index++;}
    if(module_index)fputc('\n',f);
    for(ProjectUnit*u=project->head;u;u=u->next)for(Function*fn=u->program.functions;fn;fn=fn->next)emit_signature(f,project,fn,1);
    for(ProjectUnit*u=project->head;u;u=u->next)for(ClassDecl*c=u->program.classes;c;c=c->next)for(Function*fn=c->methods;fn;fn=fn->next)emit_method(f,project,c,fn,1);
    fputc('\n',f);
    for(ProjectUnit*u=project->head;u;u=u->next)for(Function*fn=u->program.functions;fn;fn=fn->next){
        if(fn->is_external)continue;
        emit_type(f,project,fn->return_type);fputc(' ',f);emit_c_name(f,fn->name);fputc('(',f);emit_params(f,project,fn);fputs(") {\n",f);
        if(!strcmp(fn->name,"main")){size_t init_index=0;for(ProjectUnit*m=project->head;m;m=m->next){if(m->program.module_stmts)fprintf(f,"    __mbd_module_init_%zu();\n",init_index);init_index++;}}
        emit_statements(f,fn->body,1,project,NULL,fn);fputs("}\n",f);
    }
    module_index=0;
    for(ProjectUnit*u=project->head;u;u=u->next){if(u->program.module_stmts){fprintf(f,"static void __mbd_module_init_%zu(void) {\n",module_index);emit_statements(f,u->program.module_stmts,1,project,NULL,NULL);fputs("}\n",f);}module_index++;}
    for(ProjectUnit*u=project->head;u;u=u->next)for(ClassDecl*c=u->program.classes;c;c=c->next)for(Function*fn=c->methods;fn;fn=fn->next)emit_method(f,project,c,fn,0);
    if(freestanding){fputs("\n__attribute__((used, noreturn, section(\".text.start\"))) void _start(void) {\n    (void)main();\n    for (;;) __asm__ volatile(\"hlt\");\n}\n",f);}
    if(fclose(f)!=0){fprintf(stderr,"mobdrive: failed writing generated C\n");return 0;}return 1;
}
int codegen_emit_c(const Project*project,const char*path){return codegen_emit_c_mode(project,path,0);}
int codegen_emit_elf_c(const Project*project,const char*path){return codegen_emit_c_mode(project,path,1);}
