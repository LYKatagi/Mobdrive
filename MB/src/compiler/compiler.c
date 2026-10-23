#define _CRT_SECURE_NO_WARNINGS
#include <mobdrive/compiler.h>
#include <mobdrive/codegen.h>
#include <mobdrive/project.h>
#include <mobdrive/semantic.h>
#include <mobdrive/typecheck.h>
#include <mobdrive/toolchain.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *intermediate_path(const char *output_exe){
    size_t n=strlen(output_exe)+20;char*path=(char*)malloc(n);
    if(path)snprintf(path,n,"%s.mobdrive.c",output_exe);
    return path;
}
int compiler_check(const char *source){
    Project project;
    project_load(&project,source);
    int ok=typecheck_project(&project)&&semantic_validate(&project,1);
    if(ok)printf("%s: syntax and project structure are valid\n",source);
    project_free(&project);
    return ok;
}
int compiler_build(const char*source,const char*output_exe,const char*cc,char**link_options,size_t link_count){
    Project project;
    project_load(&project,source);
    int ok=typecheck_project(&project)&&semantic_validate(&project,link_count);
    char*c_source=intermediate_path(output_exe);
    if(!c_source){fprintf(stderr,"mobdrive: out of memory\n");ok=0;}
    if(ok)ok=codegen_emit_c(&project,c_source);
    if(ok)ok=toolchain_build_exe(cc,c_source,output_exe,link_options,link_count);
    if(ok){remove(c_source);printf("built %s\n",output_exe);}
    else if(c_source)fprintf(stderr,"generated C retained at '%s' for inspection\n",c_source);
    free(c_source);
    project_free(&project);
    return ok;
}
int compiler_build_elf(const char*source,const char*output_elf,const char*cc,char**link_options,size_t link_count){
    Project project;
    project_load(&project,source);
    int ok=typecheck_project(&project)&&semantic_validate(&project,link_count);
    char*c_source=intermediate_path(output_elf);
    if(!c_source){fprintf(stderr,"mobdrive: out of memory\n");ok=0;}
    if(ok)ok=codegen_emit_elf_c(&project,c_source);
    if(ok)ok=toolchain_build_elf(cc,c_source,output_elf,link_options,link_count);
    if(ok){remove(c_source);printf("built freestanding ELF %s\n",output_elf);}
    else if(c_source)fprintf(stderr,"generated C retained at '%s' for inspection\n",c_source);
    free(c_source);
    project_free(&project);
    return ok;
}
