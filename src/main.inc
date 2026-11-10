#include <sys/resource.h>
#define KILN_VERSION "2.21.0"
/* Deep Kiln recursion needs more C stack than the usual 8 MB. Linux grows the main
 * thread's stack up to the soft limit at fault time, so raising it here is enough. */
static void raise_stack_limit(void){
 struct rlimit rl;
 if(getrlimit(RLIMIT_STACK,&rl)!=0)return;
 rlim_t want=(rlim_t)512*1024*1024;
 if(rl.rlim_cur==RLIM_INFINITY||rl.rlim_cur>=want)return;
 rl.rlim_cur=(rl.rlim_max==RLIM_INFINITY||rl.rlim_max>=want)?want:rl.rlim_max;
 setrlimit(RLIMIT_STACK,&rl);
}
int main(int argc,char**argv){
 raise_stack_limit();
 if(argc<2||!strcmp(argv[1],"--help")||!strcmp(argv[1],"-h")){
  puts("Kiln " KILN_VERSION "-native\nUsage: kiln [--check] [--tree] [--disasm] <program.kiln>\n       kiln --version\n       kiln --help");
  return 0;
 }
 if(!strcmp(argv[1],"--version")){puts("Kiln " KILN_VERSION "-native");return 0;}
 bool check=false,disasm=false;const char*filename=NULL;
 for(int i=1;i<argc;i++){
  if(!strcmp(argv[i],"--check"))check=true;
  else if(!strcmp(argv[i],"--tree"))g_use_vm=false;
  else if(!strcmp(argv[i],"--disasm"))disasm=true;
  else if(!filename)filename=argv[i];
 }
 if(!filename){fprintf(stderr,"Usage: kiln [--check] [--tree] [--disasm] <program.kiln>\n");return 1;}
 if(!strchr(filename,'/'))strcpy(g_script_dir,".");else{size_t dl=(size_t)(strrchr(filename,'/')-filename);if(dl>=sizeof g_script_dir)dl=sizeof g_script_dir-1;memcpy(g_script_dir,filename,dl);g_script_dir[dl]=0;}
 FILE*f=fopen(filename,"rb");if(!f){fprintf(stderr,"kiln: cannot open %s: %s\n",filename,strerror(errno));return 1;}
 fseek(f,0,SEEK_END);long z=ftell(f);rewind(f);char*src=calloc((size_t)z+1,1);size_t got_src=fread(src,1,(size_t)z,f);src[got_src]=0;fclose(f);
 Lexer l=lex(src);Node*program=parse(&l);
 if(check){printf("OK: %s parsed by native Kiln\n",filename);return 0;}
 if(disasm){disasm_chunk(compile_body(program),filename);return 0;}
 Env*g=env_new(NULL);
 run_body(program,g);
 return 0;
}
