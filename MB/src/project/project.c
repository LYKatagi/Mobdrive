#define _CRT_SECURE_NO_WARNINGS
#include <mobdrive/project.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *copy_string(const char*s){size_t n=strlen(s)+1;char*r=(char*)malloc(n);if(r)memcpy(r,s,n);return r;}
static int file_exists(const char *path){FILE*f=fopen(path,"rb");if(!f)return 0;fclose(f);return 1;}
static char *join_path(const char *base,const char *part){
    size_t a=strlen(base),b=strlen(part);int sep=a>0&&base[a-1]!='/'&&base[a-1]!='\\';
    char*r=(char*)malloc(a+(size_t)sep+b+1);if(!r)return NULL;
    memcpy(r,base,a);if(sep)r[a++]='/';memcpy(r+a,part,b+1);
    for(char*p=r;*p;p++)if(*p=='\\')*p='/';
    return r;
}
static char *read_source(const char *path){
    FILE*f=fopen(path,"rb");if(!f)return NULL;
    if(fseek(f,0,SEEK_END)!=0){fclose(f);return NULL;}long n=ftell(f);if(n<0){fclose(f);return NULL;}rewind(f);
    char*s=(char*)malloc((size_t)n+1);if(!s){fclose(f);return NULL;}
    size_t got=fread(s,1,(size_t)n,f);s[got]=0;fclose(f);return s;
}
static char *resolve_import(const char *from,const char *part){
    if(part[0]=='/'){
        if(file_exists(part))return copy_string(part);
        const char *module=part;while(*module=='/')module++;
        const char *configured=getenv("MOBDRIVE_STDLIB");
        const char *roots[]={configured,"MB/stdlib","stdlib",NULL};
        for(size_t i=0;i<3;i++){
            if(!roots[i]||!roots[i][0])continue;
            char*candidate=join_path(roots[i],module);
            if(candidate&&file_exists(candidate))return candidate;
            free(candidate);
        }
        return join_path(configured&&configured[0]?configured:"MB/stdlib",module);
    }
    if(strlen(part)>1&&part[1]==':')return copy_string(part);
    const char *a=strrchr(from,'/'),*b=strrchr(from,'\\');const char *slash=a;
    if(!slash||(b&&b>slash))slash=b;
    size_t dir=slash?(size_t)(slash-from+1):0;
    char *r=(char*)malloc(dir+strlen(part)+1);if(!r)return NULL;
    if(dir)memcpy(r,from,dir);
    strcpy(r+dir,part);
    for(char*p=r;*p;p++)if(*p=='\\')*p='/';
    return r;
}
static int load_unit(Project *project,const char *path){
    for(ProjectUnit*u=project->head;u;u=u->next)if(!strcmp(u->path,path))return 1;
    char *source=read_source(path);
    if(!source){fprintf(stderr,"mobdrive: cannot read '%s': %s\n",path,strerror(errno));project->errors++;return 0;}
    ProjectUnit*u=(ProjectUnit*)calloc(1,sizeof(*u));if(!u){free(source);project->errors++;return 0;}
    u->path=copy_string(path);
    if(!u->path||!lexer_tokenize(source,path,&u->tokens)){free(source);free(u->path);free(u);project->errors++;return 0;}
    free(source);
    if(!parser_parse(&u->tokens,&u->program))project->errors++;
    if(!project->tail)project->tail=&project->head;
    *project->tail=u;project->tail=&u->next;
    for(Import*i=u->program.imports;i;i=i->next){
        char *child=resolve_import(path,i->source_path);
        if(!child||!load_unit(project,child))project->errors++;
        free(child);
    }
    return project->errors==0;
}
int project_load(Project *project,const char *root_source){
    memset(project,0,sizeof(*project));
    return load_unit(project,root_source);
}
void project_free(Project *project){
    ProjectUnit*u=project->head;
    while(u){ProjectUnit*n=u->next;parser_free(&u->program);lexer_free(&u->tokens);free(u->path);free(u);u=n;}
    memset(project,0,sizeof(*project));
}
