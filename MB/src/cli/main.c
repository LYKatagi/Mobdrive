#define _CRT_SECURE_NO_WARNINGS
#include <mobdrive/compiler.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int elf_suffix(const char*s){size_t n=strlen(s);return n>=4&&!strcmp(s+n-4,".elf");}
#ifdef _WIN32
static int exe_suffix(const char *s){size_t n=strlen(s);return n>=4&&s[n-4]=='.'&&tolower((unsigned char)s[n-3])=='e'&&tolower((unsigned char)s[n-2])=='x'&&tolower((unsigned char)s[n-1])=='e';}
static int native_output_name(const char *s){
    return exe_suffix(s);
}
#else
static int native_output_name(const char *s){
    size_t n=strlen(s);return n>0&&!elf_suffix(s);
}
#endif

static void usage(void){
    puts("Mobdrive compiler\nUsage:\n  mobdrive check <file.mbd>\n  mobdrive build <file.mbd> [-o program[.exe]] [--cc gcc] [--link-options object-or-library ...]\n  mobdrive build-elf <file.mbd> [-o kernel.elf] [--cc x86_64-elf-gcc] [--link-options object-or-library ...]\n  mobdrive-elf build <file.mbd> [-o kernel.elf] [--cc x86_64-elf-gcc]");
}
int main(int argc,char**argv){
    if(argc<3){usage();return 2;}const char*cmd=argv[1],*source=argv[2],*output=NULL,*cc="gcc";int elf_build=!strcmp(cmd,"build-elf")||(strstr(argv[0],"mobdrive-elf")&&!strcmp(cmd,"build"));char**links=(char**)calloc((size_t)argc,sizeof(char*));size_t link_count=0;
    if(!strcmp(cmd,"check")){if(argc!=3){usage();free(links);return 2;}int ok=compiler_check(source);free(links);return ok?0:1;}
    if(strcmp(cmd,"build")&&strcmp(cmd,"build-elf")){usage();free(links);return 2;}
    output=elf_build?"kernel.elf":
#ifdef _WIN32
        "a.exe";
#else
        "a.out";
#endif
    for(int i=3;i<argc;i++){
        if(!strcmp(argv[i],"-o")&&i+1<argc)output=argv[++i];
        else if(!strcmp(argv[i],"--cc")&&i+1<argc)cc=argv[++i];
        else if(!strcmp(argv[i],"--link-options")&&i+1<argc)links[link_count++]=argv[++i];
        else {fprintf(stderr,"mobdrive: unknown or incomplete option '%s'\n",argv[i]);usage();free(links);return 2;}
    }
    if(elf_build?!elf_suffix(output):!native_output_name(output)){fprintf(stderr,"mobdrive: invalid output filename for this target\n");free(links);return 2;}
    int ok=elf_build?compiler_build_elf(source,output,cc,links,link_count):compiler_build(source,output,cc,links,link_count);free(links);return ok?0:1;
}
