#include <mobdrive/semantic.h>
#include <stdio.h>
#include <string.h>

static ClassDecl *find_class(const Project*p,const char*name){for(ProjectUnit*u=p->head;u;u=u->next)for(ClassDecl*c=u->program.classes;c;c=c->next)if(!strcmp(c->name,name))return c;return NULL;}
static InterfaceDecl *find_interface(const Project*p,const char*name){for(ProjectUnit*u=p->head;u;u=u->next)for(InterfaceDecl*i=u->program.interfaces;i;i=i->next)if(!strcmp(i->name,name))return i;return NULL;}
static Function *find_method(const ClassDecl*c,const char*name){for(Function*f=c?c->methods:NULL;f;f=f->next)if(!strcmp(f->name,name))return f;return NULL;}
static int same_signature(const Function*a,const Function*b){
    if(strcmp(a->return_type,b->return_type))return 0;
    Param*x=a->params,*y=b->params;
    while(x&&y){if(strcmp(x->type,y->type))return 0;x=x->next;y=y->next;}return !x&&!y;
}

int semantic_validate(const Project *project,size_t link_option_count){
    int ok=project->errors==0;
    for(ProjectUnit*u=project->head;u;u=u->next)for(Function*f=u->program.functions;f;f=f->next){
        int first=1;
        for(ProjectUnit*v=project->head;v;v=v->next)for(Function*g=v->program.functions;g;g=g->next){
            if(g==f)break;
            if(!strcmp(f->name,g->name))first=0;
        }
        if(!first){fprintf(stderr,"%s: error: duplicate function '%s'\n",u->path,f->name);ok=0;}
        if(f->is_external&&f->link_mode&&!strcmp(f->link_mode,"nolink")&&!link_option_count){
            fprintf(stderr,"%s: error: bin function '%s' uses nolink; provide --link-options <object-or-library>\n",u->path,f->name);ok=0;
        }
    }
    for(ProjectUnit*u=project->head;u;u=u->next)for(CheckRule*r=u->program.checks;r;r=r->next)for(size_t i=0;i<r->count;i++){
        int found=0;
        for(ProjectUnit*v=project->head;v;v=v->next)for(Function*f=v->program.functions;f;f=f->next)if(!strcmp(f->name,r->names[i]))found=1;
        if(!found){fprintf(stderr,"%s: error: nochk names unknown function '%s'\n",u->path,r->names[i]);ok=0;}
    }
    for(ProjectUnit*u=project->head;u;u=u->next)for(ClassDecl*c=u->program.classes;c;c=c->next){
        if(c->base_name&&!find_class(project,c->base_name)){fprintf(stderr,"%s: error: unknown base class '%s'\n",u->path,c->base_name);ok=0;}
        size_t class_count=0;for(ProjectUnit*v=project->head;v;v=v->next)for(ClassDecl*k=v->program.classes;k;k=k->next)class_count++;
        ClassDecl*base=c;for(size_t step=0;base&&base->base_name&&step<=class_count;step++){base=find_class(project,base->base_name);if(base==c){fprintf(stderr,"%s: error: inheritance cycle involving '%s'\n",u->path,c->name);ok=0;break;}}
        for(NameRef*r=c->interfaces;r;r=r->next){InterfaceDecl*i=find_interface(project,r->name);if(!i){fprintf(stderr,"%s: error: unknown interface '%s'\n",u->path,r->name);ok=0;continue;}
            for(Function*required=i->methods;required;required=required->next){Function*provided=find_method(c,required->name);if(!provided||!same_signature(provided,required)){fprintf(stderr,"%s: error: class '%s' does not implement interface method '%s' with a matching signature\n",u->path,c->name,required->name);ok=0;}}
        }
        for(Function*f=c->methods;f;f=f->next)for(Function*g=f->next;g;g=g->next)if(!strcmp(f->name,g->name)){fprintf(stderr,"%s: error: duplicate method '%s' in class '%s'\n",u->path,f->name,c->name);ok=0;}
    }
    for(ProjectUnit*u=project->head;u;u=u->next)for(ClassDecl*c=u->program.classes;c;c=c->next)for(ProjectUnit*v=project->head;v;v=v->next)for(ClassDecl*d=v->program.classes;d;d=d->next)if(c!=d&&!strcmp(c->name,d->name)){fprintf(stderr,"%s: error: duplicate class '%s'\n",u->path,c->name);ok=0;}
    for(ProjectUnit*u=project->head;u;u=u->next)for(InterfaceDecl*i=u->program.interfaces;i;i=i->next)for(ProjectUnit*v=project->head;v;v=v->next)for(InterfaceDecl*j=v->program.interfaces;j;j=j->next)if(i!=j&&!strcmp(i->name,j->name)){fprintf(stderr,"%s: error: duplicate interface '%s'\n",u->path,i->name);ok=0;}
    return ok;
}
