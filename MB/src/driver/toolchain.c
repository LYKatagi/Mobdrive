#include <mobdrive/toolchain.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <process.h>
#else
#include <spawn.h>
#include <sys/wait.h>
extern char **environ;
#endif

static int run_compiler(const char*cc,const char*c_source,const char*output,char**links,size_t count,int elf){
    char**args=(char**)calloc(count+16,sizeof(char*));if(!args)return 0;
    size_t n=0;args[n++]=(char*)cc;args[n++]="-std=c11";args[n++]="-O3";args[n++]="-flto";
    if(elf){args[n++]="-ffreestanding";args[n++]="-fno-stack-protector";args[n++]="-fno-pie";args[n++]="-no-pie";args[n++]="-nostdlib";args[n++]="-march=x86-64";args[n++]="-Wl,-e,_start";args[n++]="-Wl,--build-id=none";}
    else args[n++]="-march=native";
    args[n++]=(char*)c_source;args[n++]="-o";args[n++]=(char*)output;
    for(size_t i=0;i<count;i++)args[n++]=links[i];
    args[n]=NULL;
#ifdef _WIN32
    int ok=_spawnvp(_P_WAIT,cc,(const char * const*)args)==0;
#else
    pid_t pid;int status=0;int spawned=posix_spawnp(&pid,cc,NULL,NULL,args,environ)==0;
    int ok=spawned&&waitpid(pid,&status,0)>=0&&WIFEXITED(status)&&WEXITSTATUS(status)==0;
#endif
    free(args);
    if(!ok)fprintf(stderr,"mobdrive: native C compiler failed\n");
    return ok;
}
int toolchain_build_exe(const char*cc,const char*c_source,const char*output_exe,char**links,size_t count){return run_compiler(cc,c_source,output_exe,links,count,0);}
int toolchain_build_elf(const char*cc,const char*c_source,const char*output_elf,char**links,size_t count){
    if(!run_compiler(cc,c_source,output_elf,links,count,1))return 0;
    FILE*f=fopen(output_elf,"rb");unsigned char magic[4]={0};size_t n=f?fread(magic,1,sizeof magic,f):0;if(f)fclose(f);
    if(n!=4||magic[0]!=0x7f||magic[1]!='E'||magic[2]!='L'||magic[3]!='F'){
        remove(output_elf);
        fprintf(stderr,"mobdrive: selected compiler did not produce ELF; use an ELF-targeting compiler such as x86_64-elf-gcc\n");
        return 0;
    }
    return 1;
}
