static char* resolve_module(const char*name){
 char path[4096];FILE*f=fopen(name,"rb");if(f){fclose(f);return strdup(name);}
 static char libdir[4200];static char kp[4096];const char*dirs[8];int nd=0;
 dirs[nd++]=g_script_dir;
 const char*env=getenv("KILN_PATH");
 if(env){strncpy(kp,env,sizeof kp-1);kp[sizeof kp-1]=0;for(char*tok=strtok(kp,":");tok&&nd<7;tok=strtok(NULL,":"))dirs[nd++]=tok;}
 char exe[4096];ssize_t k=readlink("/proc/self/exe",exe,sizeof exe-1);
 if(k>0){exe[k]=0;char*sl=strrchr(exe,'/');if(sl){*sl=0;snprintf(libdir,sizeof libdir,"%s/lib",exe);dirs[nd++]=libdir;static char libdir2[4200];snprintf(libdir2,sizeof libdir2,"%s/../lib",exe);dirs[nd++]=libdir2;}}
 for(int i=0;i<nd;i++)for(int suffix=0;suffix<2;suffix++){snprintf(path,sizeof path,"%s/%s%s",dirs[i],name,suffix?".kiln":"");f=fopen(path,"rb");if(f){fclose(f);return strdup(path);}}
 return NULL;}
static bool contains_closure(Node*n){if(!n)return false;if(n->t==N_LAMBDA||n->t==N_FUN||n->t==N_CLASS)return true;if(contains_closure(n->a)||contains_closure(n->b)||contains_closure(n->c)||contains_closure(n->next))return true;for(int i=0;i<n->argc;i++)if(contains_closure(n->args[i]))return true;return false;}
static void index_assign(Value base,Value idx,Value v){
 if(base.t==V_LIST){base.list->items[list_index(idx,base.list->len)]=v;return;}
 if(base.t==V_DICT){char*k=stringify(idx);dict_set(base.dict,k,v);return;}
 if(base.t==V_INST){dict_set(((Instance*)base.ptr)->fields.dict,stringify(idx),v);return;}
 rt_err("indexed assignment needs a list or dict\n");}
static void assign_target(Node*t,Value v,Env*e){
 if(t->t==N_VAR){set(e,t->name,v);return;}
 if(t->t==N_INDEX){Value base=eval(t->a,e),idx=eval(t->b,e);index_assign(base,idx,v);return;}
 if(t->t==N_EXPR){Value base=eval(t->a,e);index_assign(base,str(t->name),v);return;}
 rt_err("invalid assignment target\n");}
static bool run_protected(Node*blk,Env*e){
 if(try_depth>=KILN_MAX_TRY_DEPTH){rt_err("too many nested try blocks\n");exit(3);}int my=try_depth;int sd=loop_depth,sf=loop_floor,sl=loop_signal,sc=call_depth;bool sr=returning;
 jmp_buf jb;try_stack[try_depth++]=&jb;
 if(setjmp(jb)==0){for(Node*x=blk->next;x&&!returning&&!loop_signal;x=x->next)eval(x,e);try_depth=my;return false;}
 try_depth=my;loop_depth=sd;loop_floor=sf;loop_signal=sl;call_depth=sc;returning=sr;
 return true;}
static void run_finally(Node*blk,Env*e){
 Env*fe=env_new(e);
 for(Node*x=blk->next;x;x=x->next)eval(x,fe);}
static Env* block_env(Node*blk,Env*e){if(!blk->opc){blk->opc=1;for(Node*x=blk->next;x;x=x->next)if(x->t==N_LET||x->t==N_FUN||x->t==N_CLASS||x->t==N_IMPORT)blk->opc=2;}return blk->opc==2?env_new(e):e;}
static Value unop(Node*n,Value v){if(!strcmp(n->op,"not"))return boolean(!truth(v));if(v.t!=V_NUM){rt_err("unary operator needs number\n");exit(3);}return num(!strcmp(n->op,"-")?-v.n:v.n);}
static Value binop(Node*n,Value a,Value b){  switch(n->opc){
  case OP_IN:return boolean(member_of(a,b));
  case OP_ADD:if(a.t==V_STR||b.t==V_STR)return str_concat(a,b);need_numbers(a,b,n->op);return num(a.n+b.n);
  case OP_SUB:need_numbers(a,b,n->op);return num(a.n-b.n);
  case OP_MUL:need_numbers(a,b,n->op);return num(a.n*b.n);
  case OP_DIV:need_numbers(a,b,n->op);if(b.n==0)rt_err("division by zero\n");return num(a.n/b.n);
  case OP_FLOORDIV:need_numbers(a,b,n->op);if(b.n==0)rt_err("division by zero\n");return num(floor(a.n/b.n));
  case OP_MOD:{need_numbers(a,b,n->op);if(b.n==0)rt_err("modulo by zero\n");double r=fmod(a.n,b.n);if(r!=0&&((r<0)!=(b.n<0)))r+=b.n;return num(r);}
  case OP_POW:need_numbers(a,b,n->op);return num(pow(a.n,b.n));
  case OP_NOTIN:return boolean(!member_of(a,b));
  case OP_EQ:return boolean(values_equal(a,b));
  case OP_NE:return boolean(!values_equal(a,b));
  case OP_LT:case OP_GT:case OP_LE:case OP_GE:{
   int c;
   if(a.t==V_STR&&b.t==V_STR)c=str_cmp(a,b);
   else{need_numbers(a,b,n->op);c=a.n<b.n?-1:(a.n>b.n?1:0);}
   if(n->opc==OP_LT)return boolean(c<0);
   if(n->opc==OP_GT)return boolean(c>0);
   if(n->opc==OP_LE)return boolean(c<=0);
   return boolean(c>=0);}
  default:break;
  }
  rt_err("unknown operator %s\n",n->op);return nul();}
static Value eval(Node*n,Env*e){if(!n)return nul();if(n->line)g_line=n->line;switch(n->t){case N_NUM:case N_STR:case N_BOOL:return n->val;case N_NULL:return nul();case N_LIST:{Value v=list_value();for(int i=0;i<n->argc;i++)list_push(v.list,eval(n->args[i],e));return v;}case N_DICT:{Value v=dict_value();for(int i=0;i<n->argc;i+=2){Value k=eval(n->args[i],e);char*key=stringify(k);dict_set(v.dict,key,eval(n->args[i+1],e));}return v;}case N_SLICE:{Value base=eval(n->a,e);long L;if(base.t==V_STR)L=(long)text_len(base);else if(base.t==V_LIST)L=(long)base.list->len;else{rt_err("slices work on text and lists\n");exit(3);}
  long s=0,t=L;
  if(n->b){Value v=eval(n->b,e);if(v.t!=V_NUM){rt_err("slice start must be a number\n");exit(3);}s=(long)v.n;}
  if(n->c){Value v=eval(n->c,e);if(v.t!=V_NUM){rt_err("slice end must be a number\n");exit(3);}t=(long)v.n;}
  if(s<0){s+=L;}if(t<0){t+=L;}if(s<0){s=0;}if(t>L){t=L;}if(s>L){s=L;}if(t<s){t=s;}
  if(base.t==V_STR){size_t bs=text_offset(base,(size_t)s),bt=text_offset(base,(size_t)t);return strv_n(base.s+bs,bt-bs);}
  Value r=list_value();for(long i=s;i<t;i++)list_push(r.list,base.list->items[i]);return r;}case N_INDEX:{Value base=eval(n->a,e),idx=eval(n->b,e);if(base.t==V_LIST)return base.list->items[list_index(idx,base.list->len)];if(base.t==V_DICT){char*k=stringify(idx);long i=dict_find(base.dict,k);if(i<0){rt_err("missing dictionary key '%s'\n",k);exit(3);}return base.dict->values[i];}if(base.t==V_INST)return instance_member(base,stringify(idx));if(base.t==V_STR&&idx.t==V_NUM){long i=list_index(idx,text_len(base));size_t off=text_offset(base,(size_t)i);return strv_n(base.s+off,utf8_char_width(base.s,off,base.len));}rt_err("value is not indexable\n");exit(3);}case N_INDEX_SET:{Value base=eval(n->a,e),idx=eval(n->b,e),v=eval(n->c,e);index_assign(base,idx,v);return v;}case N_COND:return truth(eval(n->a,e))?eval(n->b,e):eval(n->c,e);
case N_UNPACK:{
 int count=n->argc;
 Value*vals=calloc((size_t)count,sizeof(Value));
 if(n->c){
  if(n->c->argc!=count){rt_err("unpacking needs %d values, got %d\n",count,n->c->argc);}
  for(int i=0;i<count;i++)vals[i]=eval(n->c->args[i],e);
 }else{
  Value v=eval(n->a,e);
  if(v.t!=V_LIST){rt_err("unpacking needs a list of %d values\n",count);}
  if((int)v.list->len!=count){rt_err("unpacking needs %d values, got %d\n",count,(int)v.list->len);}
  for(int i=0;i<count;i++)vals[i]=v.list->items[i];
 }
 for(int i=0;i<count;i++){if(n->opc==1)define(e,n->args[i]->name,vals[i]);else assign_target(n->args[i],vals[i],e);}
 return nul();}
case N_VAR:return get(e,n->name);case N_LET:{Value v=eval(n->a,e);define(e,n->name,v);return v;}case N_SET:{Value v=eval(n->a,e);set(e,n->name,v);return v;}case N_PRINT:{Buf b={0};for(int i=0;i<n->argc;i++){if(i)buf_str(&b," ");buf_str(&b,stringify(eval(n->args[i],e)));}puts(b.s?b.s:"");return nul();}case N_IF:if(truth(eval(n->a,e))){Env*l=block_env(n->b,e);for(Node*x=n->b->next;x&&!returning&&!loop_signal;x=x->next)eval(x,l);}else if(n->c){Env*l=block_env(n->c,e);for(Node*x=n->c->next;x&&!returning&&!loop_signal;x=x->next)eval(x,l);}return nul();case N_WHILE:{loop_depth++;while(truth(eval(n->a,e))){Env*l=block_env(n->b,e);for(Node*x=n->b->next;x&&!returning&&!loop_signal;x=x->next)eval(x,l);if(returning)break;if(loop_signal==1){loop_signal=0;break;}if(loop_signal==2)loop_signal=0;}loop_depth--;return nul();}case N_FOR:{Value seq=eval(n->a,e);size_t count=0;if(seq.t==V_LIST)count=seq.list->len;else if(seq.t==V_STR)count=seq.len;else if(seq.t==V_DICT)count=seq.dict->len;else{rt_err("for-in needs a list, string, or dictionary\n");exit(3);}if(!n->opc){n->opc=contains_closure(n->b)?2:1;for(Node*x=n->b->next;x;x=x->next)if(x->t==N_LET||x->t==N_FUN||x->t==N_CLASS||x->t==N_IMPORT)n->opc=2;}
 loop_depth++;Env*l=NULL;size_t step=1;for(size_t i=0;i<count&&!returning;i+=step){step=1;if(!l||n->opc!=1)l=env_new(e);Value item;if(seq.t==V_STR){step=utf8_char_width(seq.s,i,seq.len);item=strv_n(seq.s+i,step);}else item=seq.t==V_LIST?seq.list->items[i]:str(seq.dict->keys[i]);define(l,n->name,item);for(Node*x=n->b->next;x&&!returning&&!loop_signal;x=x->next)eval(x,l);if(loop_signal==1){loop_signal=0;break;}if(loop_signal==2)loop_signal=0;}loop_depth--;return nul();}case N_BREAK:if(loop_depth<=loop_floor){rt_err("break used outside a loop\n");exit(3);}loop_signal=1;return nul();case N_CONTINUE:if(loop_depth<=loop_floor){rt_err("continue used outside a loop\n");exit(3);}loop_signal=2;return nul();case N_BLOCK:{Env*l=block_env(n,e);for(Node*x=n->next;x&&!returning&&!loop_signal;x=x->next)eval(x,l);return nul();}case N_FSTR:{Buf b={0};for(int i=0;i<n->argc;i++){Value v=eval(n->args[i],e);char*x;if(n->params[i]&&v.t==V_NUM){char tmp[128];snprintf(tmp,sizeof tmp,"%.*f",atoi(n->params[i]+1),v.n);x=strdup(tmp);}else x=stringify(v);buf_str(&b,x);}return str(b.s?b.s:"");}case N_THROW:{Value v=eval(n->a,e);char*s=stringify(v);snprintf(err_text,sizeof err_text,"%s",s);err_line=g_line;raise_text();return nul();}case N_TRY:{
 bool failed=run_protected(n->a,env_new(e));
 if(!failed){if(n->c)run_finally(n->c,e);return nul();}
 if(!n->b){char*msg=strdup(err_text);int line=err_line;if(n->c)run_finally(n->c,e);strcpy(err_text,msg);err_line=line;raise_text();return nul();}
 Env*cenv=env_new(e);if(n->name)define(cenv,n->name,str(err_text));
 if(n->c){
  if(run_protected(n->b,cenv)){char*msg=strdup(err_text);int line=err_line;run_finally(n->c,e);strcpy(err_text,msg);err_line=line;raise_text();return nul();}
  run_finally(n->c,e);return nul();
 }
 for(Node*x=n->b->next;x&&!returning&&!loop_signal;x=x->next)eval(x,cenv);
 return nul();}
case N_CLASS:{Class*c=calloc(1,sizeof(Class));c->name=n->name;c->methods=n->args;c->mcount=n->argc;c->env=e;Value v={.t=V_CLASS,.ptr=c};define(e,n->name,v);return v;}case N_LAMBDA:{Func*f=calloc(1,sizeof(Func));f->names=n->params;f->count=n->pc;f->body=n->a;f->closure=e;f->defaults=n->args;Value v={.t=V_FUNC,.f=f};return v;}case N_FUN:{Func*f=calloc(1,sizeof(Func));f->names=n->params;f->count=n->pc;f->body=n->a;f->closure=e;f->defaults=n->args;Value v={.t=V_FUNC,.f=f};define(e,n->name,v);return v;}case N_RET:{Value rv=n->a?eval(n->a,e):nul();retvalue=rv;returning=true;return rv;}case N_IMPORT:{
 const char*name=n->name;
 bool builtin=!strcmp(name,"math")||!strcmp(name,"string")||!strcmp(name,"fs")||!strcmp(name,"system")||!strcmp(name,"sqlite")||!strcmp(name,"gui")||!strcmp(name,"time")||!strcmp(name,"path")||!strcmp(name,"json")||!strcmp(name,"http")||!strcmp(name,"regex");
 ModRef*mref=calloc(1,sizeof(ModRef));mref->name=strdup(name);Value v={.t=V_MOD,.ptr=mref};
 if(!builtin){
   char*found=resolve_module(name);
   FILE*mf=found?fopen(found,"rb"):NULL;
   if(!mf){fprintf(stderr,"Kiln import error: cannot open module '%s': %s\n",name,strerror(errno));exit(4);}
   if(fseek(mf,0,SEEK_END)!=0){fprintf(stderr,"Kiln import error: cannot seek module '%s'\n",name);exit(4);}
   long msize=ftell(mf);rewind(mf);
   if(msize<0||msize>16*1024*1024){fprintf(stderr,"Kiln import error: module '%s' is too large\n",name);exit(4);}
   char*msrc=calloc((size_t)msize+1,1);
   if(!msrc||fread(msrc,1,(size_t)msize,mf)!=(size_t)msize){fprintf(stderr,"Kiln import error: could not read module '%s'\n",name);fclose(mf);exit(4);}
   fclose(mf);
   Lexer ml=lex(msrc);Node*mp=parse(&ml);Env*me=env_new(NULL);
   run_body(mp,me);
   mref->env=me;
 }
 define(e,n->op,v);return v;}case N_EXPR:{Value base=eval(n->a,e);if(n->name&&base.t==V_MOD&&((ModRef*)base.ptr)->env)return get(((ModRef*)base.ptr)->env,n->name);if(n->name&&base.t==V_INST)return instance_member(base,n->name);return base;}case N_UN:return unop(n,eval(n->a,e));case N_BIN:{Value a=eval(n->a,e);if(n->opc==OP_AND)return truth(a)?eval(n->b,e):a;if(n->opc==OP_OR)return truth(a)?a:eval(n->b,e);Value b=eval(n->b,e);return binop(n,a,b);}
 case N_CALL:{Value*args=calloc((size_t)n->argc,sizeof(Value));for(int i=0;i<n->argc;i++)args[i]=eval(n->args[i],e);if(n->a->t==N_EXPR&&n->a->a->t==N_VAR){Value mod=get(e,n->a->a->name);if(mod.t==V_MOD){ModRef*mr=(ModRef*)mod.ptr;if(mr->env){Env*module_env=mr->env;Value target=get(module_env,n->a->name);return call(target,args,n->argc,module_env);}return module_call(mr->name,n->a->name,args,n->argc);}
#ifdef KILN_WITH_SQLITE
 if(mod.t==V_DB){sqlite3*db=(sqlite3*)mod.ptr;if(!strcmp(n->a->name,"exec")){if(n->argc!=1){rt_err("db.exec expects one argument\n");exit(3);}char*err=NULL;int rc=sqlite3_exec(db,cstr(args[0]),NULL,NULL,&err);if(rc!=SQLITE_OK){fprintf(stderr,"Kiln sqlite error: %s\n",err?err:sqlite3_errmsg(db));sqlite3_free(err);exit(3);}return boolean(true);}if(!strcmp(n->a->name,"scalar")){if(n->argc!=1){rt_err("db.scalar expects one argument\n");exit(3);}sqlite3_stmt*st=NULL;if(sqlite3_prepare_v2(db,cstr(args[0]),-1,&st,NULL)!=SQLITE_OK){fprintf(stderr,"Kiln sqlite error: %s\n",sqlite3_errmsg(db));exit(3);}Value out=nul();if(sqlite3_step(st)==SQLITE_ROW){const unsigned char*v=sqlite3_column_text(st,0);if(v)out=str((const char*)v);}sqlite3_finalize(st);return out;}if(!strcmp(n->a->name,"changes")){if(n->argc){rt_err("db.changes expects no arguments\n");exit(3);}return num(sqlite3_changes(db));}if(!strcmp(n->a->name,"last_id")){if(n->argc){rt_err("db.last_id expects no arguments\n");exit(3);}return num((double)sqlite3_last_insert_rowid(db));}if(!strcmp(n->a->name,"close")){if(n->argc){rt_err("db.close expects no arguments\n");exit(3);}int rc=sqlite3_close(db);if(rc!=SQLITE_OK){fprintf(stderr,"Kiln sqlite error: %s\n",sqlite3_errstr(rc));exit(3);}return boolean(true);}}
#endif
}
if(n->a->t==N_EXPR){
 Value receiver=(n->a->a->t==N_VAR)?get(e,n->a->a->name):eval(n->a->a,e);const char*method=n->a->name;
 if(receiver.t==V_INST){return call(instance_member(receiver,method),args,n->argc,e);}
 if(receiver.t==V_LIST){
   if(!strcmp(method,"append")){if(n->argc!=1){rt_err("list.append expects one argument\n");exit(3);}list_push(receiver.list,args[0]);return nul();}
   if(!strcmp(method,"pop")){if(n->argc>1||receiver.list->len==0){rt_err("list.pop expects an optional index and a non-empty list\n");exit(3);}size_t i=n->argc?(size_t)list_index(args[0],receiver.list->len):receiver.list->len-1;Value out=receiver.list->items[i];for(size_t j=i+1;j<receiver.list->len;j++)receiver.list->items[j-1]=receiver.list->items[j];receiver.list->len--;return out;}
   if(!strcmp(method,"length")){if(n->argc){rt_err("list.length expects no arguments\n");exit(3);}return num(receiver.list->len);}
   if(!strcmp(method,"map")||!strcmp(method,"filter")||!strcmp(method,"for_each")||!strcmp(method,"find")||!strcmp(method,"any")||!strcmp(method,"all")){if(n->argc!=1){rt_err("list.%s expects one function\n",method);exit(3);}Value out=list_value();for(size_t i=0;i<receiver.list->len;i++){Value item=receiver.list->items[i];Value r=call(args[0],&item,1,e);bool t=truth(r);if(!strcmp(method,"map"))list_push(out.list,r);else if(!strcmp(method,"filter")){if(t)list_push(out.list,item);}else if(!strcmp(method,"find")){if(t)return item;}else if(!strcmp(method,"any")){if(t)return boolean(true);}else if(!strcmp(method,"all")){if(!t)return boolean(false);}}if(!strcmp(method,"map")||!strcmp(method,"filter"))return out;if(!strcmp(method,"find"))return nul();if(!strcmp(method,"any"))return boolean(false);if(!strcmp(method,"all"))return boolean(true);return nul();}
   if(!strcmp(method,"reduce")){if(n->argc!=2){rt_err("list.reduce expects a function and an initial value\n");exit(3);}Value acc=args[1];for(size_t i=0;i<receiver.list->len;i++){Value pair[2]={acc,receiver.list->items[i]};acc=call(args[0],pair,2,e);}return acc;}
   if(!strcmp(method,"sum")||!strcmp(method,"min")||!strcmp(method,"max")){if(n->argc){rt_err("list.%s expects no arguments\n",method);exit(3);}size_t len=receiver.list->len;if(!strcmp(method,"sum")){double total=0;for(size_t i=0;i<len;i++){if(receiver.list->items[i].t!=V_NUM){rt_err("list.sum needs numbers\n");exit(3);}total+=receiver.list->items[i].n;}return num(total);}if(len==0){rt_err("list.%s of an empty list\n",method);exit(3);}double best=0;for(size_t i=0;i<len;i++){Value v=receiver.list->items[i];if(v.t!=V_NUM){rt_err("list.%s needs numbers\n",method);exit(3);}if(i==0||(!strcmp(method,"min")?v.n<best:v.n>best))best=v.n;}return num(best);}
   if(!strcmp(method,"clear")){if(n->argc){rt_err("list.clear expects no arguments\n");exit(3);}receiver.list->len=0;return nul();}
   if(!strcmp(method,"reverse")){if(n->argc){rt_err("list.reverse expects no arguments\n");exit(3);}for(size_t i=0,j=receiver.list->len;i+1<j;i++,j--){Value t=receiver.list->items[i];receiver.list->items[i]=receiver.list->items[j-1];receiver.list->items[j-1]=t;}return nul();}
   if(!strcmp(method,"sort")){if(n->argc){rt_err("list.sort expects no arguments\n");exit(3);}size_t len=receiver.list->len;if(len>1){Value t0=receiver.list->items[0];for(size_t i=1;i<len;i++){Value v=receiver.list->items[i];if(v.t!=t0.t||(v.t!=V_NUM&&v.t!=V_STR)){rt_err("list.sort needs a list of all numbers or all strings\n");exit(3);}}qsort(receiver.list->items,len,sizeof(Value),compare_values);}return nul();}
   if(!strcmp(method,"index_of")){if(n->argc!=1){rt_err("list.index_of expects one value\n");exit(3);}for(size_t i=0;i<receiver.list->len;i++)if(values_equal(receiver.list->items[i],args[0]))return num((double)i);return num(-1);}
   if(!strcmp(method,"contains")){if(n->argc!=1){rt_err("list.contains expects one value\n");exit(3);}for(size_t i=0;i<receiver.list->len;i++)if(values_equal(receiver.list->items[i],args[0]))return boolean(true);return boolean(false);}
   if(!strcmp(method,"join")){if(n->argc>1){rt_err("list.join expects an optional separator\n");exit(3);}const char*sep=n->argc?stringify(args[0]):"";size_t total=1;char**parts=calloc(receiver.list->len+1,sizeof(char*));for(size_t i=0;i<receiver.list->len;i++){parts[i]=stringify(receiver.list->items[i]);total+=strlen(parts[i])+strlen(sep);}char*out=calloc(total,1);for(size_t i=0;i<receiver.list->len;i++){if(i)strcat(out,sep);strcat(out,parts[i]);}free(parts);return str(out);}
 }
 if(receiver.t==V_DICT){
   if(!strcmp(method,"set")){if(n->argc!=2){rt_err("dict.set expects key and value\n");exit(3);}char*k=stringify(args[0]);dict_set(receiver.dict,k,args[1]);return args[1];}
   if(!strcmp(method,"get")){if(n->argc!=1){rt_err("dict.get expects one key\n");exit(3);}char*k=stringify(args[0]);long i=dict_find(receiver.dict,k);return i<0?nul():receiver.dict->values[i];}
   if(!strcmp(method,"keys")||!strcmp(method,"values")){Value r=list_value();bool keys=!strcmp(method,"keys");for(size_t i=0;i<receiver.dict->len;i++)list_push(r.list,keys?str(receiver.dict->keys[i]):receiver.dict->values[i]);return r;}
 if(!strcmp(method,"has")){if(n->argc!=1){rt_err("dict.has expects one key\n");exit(3);}char*k=stringify(args[0]);return boolean(dict_find(receiver.dict,k)>=0);}
   if(!strcmp(method,"length")){if(n->argc){rt_err("dict.length expects no arguments\n");exit(3);}return num(receiver.dict->len);}
 }
 if(receiver.t==V_STR){Value*full=calloc((size_t)n->argc+1,sizeof(Value));full[0]=receiver;for(int i=0;i<n->argc;i++)full[i+1]=args[i];return module_call("string",method,full,n->argc+1);}
 if(n->a->a->t!=N_VAR&&receiver.t!=V_LIST&&receiver.t!=V_DICT){rt_err("value has no method '%s'\n",method);exit(3);}
}
if(n->opc==1&&!strcmp(n->a->name,"range")&&!env_has(e,n->a->name)){if(n->argc<1||n->argc>3){rt_err("range expects 1 to 3 arguments\n");exit(3);}double start=0,stop=0,step=1;if(n->argc==1)stop=args[0].n;else{start=args[0].n;stop=args[1].n;if(n->argc==3)step=args[2].n;}if(args[0].t!=V_NUM||(n->argc>1&&args[1].t!=V_NUM)||(n->argc>2&&args[2].t!=V_NUM)||floor(start)!=start||floor(stop)!=stop||floor(step)!=step||step==0){rt_err("range arguments must be integers and step cannot be zero\n");exit(3);}Value out=list_value();size_t guard=0;for(double v=start;(step>0?v<stop:v>stop);v+=step){if(++guard>1000000){rt_err("range exceeds one million items\n");exit(3);}list_push(out.list,num(v));}return out;}if(n->opc==1&&!strcmp(n->a->name,"input")&&!env_has(e,n->a->name)){if(n->argc>1){rt_err("input accepts zero or one argument\n");exit(3);}if(n->argc){char*p=stringify(args[0]);fputs(p,stdout);fflush(stdout);}char line[4096];if(!fgets(line,sizeof line,stdin))return str("");line[strcspn(line,"\r\n")]=0;return str(line);}if(n->opc==1&&!strcmp(n->a->name,"type")&&!env_has(e,n->a->name)){if(n->argc!=1){rt_err("type expects one argument\n");exit(3);}const char*t=args[0].t==V_NUM?"number":args[0].t==V_BOOL?"bool":args[0].t==V_STR?"string":args[0].t==V_FUNC?"function":args[0].t==V_MOD?"module":args[0].t==V_DB?"database":args[0].t==V_INST?"instance":args[0].t==V_CLASS?"class":args[0].t==V_LIST?"list":args[0].t==V_DICT?"dict":"null";return str(t);}if(n->opc==1&&!strcmp(n->a->name,"int")&&!env_has(e,n->a->name)){if(n->argc!=1){rt_err("int expects one argument\n");exit(3);}return num(trunc(args[0].t==V_STR?strtod(cstr(args[0]),NULL):args[0].n));}if(n->opc==1&&!strcmp(n->a->name,"float")&&!env_has(e,n->a->name)){if(n->argc!=1){rt_err("float expects one argument\n");exit(3);}return num(args[0].t==V_STR?strtod(cstr(args[0]),NULL):args[0].n);}if(n->opc==1&&!strcmp(n->a->name,"assert")&&!env_has(e,n->a->name)){if(n->argc<1||n->argc>2)rt_err("assert expects a condition and an optional message\n");if(!truth(args[0])){if(n->argc==2)rt_err("assertion failed: %s\n",stringify(args[1]));rt_err("assertion failed\n");}return nul();}
 if(n->opc==1&&!strcmp(n->a->name,"enumerate")&&!env_has(e,n->a->name)){if(n->argc!=1||args[0].t!=V_LIST)rt_err("enumerate expects one list\n");Value r=list_value();for(size_t i=0;i<args[0].list->len;i++){Value pair=list_value();list_push(pair.list,num((double)i));list_push(pair.list,args[0].list->items[i]);list_push(r.list,pair);}return r;}
 if(n->opc==1&&!strcmp(n->a->name,"zip")&&!env_has(e,n->a->name)){if(n->argc!=2||args[0].t!=V_LIST||args[1].t!=V_LIST)rt_err("zip expects two lists\n");size_t k=args[0].list->len<args[1].list->len?args[0].list->len:args[1].list->len;Value r=list_value();for(size_t i=0;i<k;i++){Value pair=list_value();list_push(pair.list,args[0].list->items[i]);list_push(pair.list,args[1].list->items[i]);list_push(r.list,pair);}return r;}
 if(n->opc==1&&(!strcmp(n->a->name,"min")||!strcmp(n->a->name,"max"))&&!env_has(e,n->a->name)){bool want_max=!strcmp(n->a->name,"max");Value*items=args;size_t count=(size_t)n->argc;if(n->argc==1&&args[0].t==V_LIST){items=args[0].list->items;count=args[0].list->len;}if(count==0)rt_err("%s needs at least one number\n",n->a->name);for(size_t i=0;i<count;i++)if(items[i].t!=V_NUM)rt_err("%s expects numbers\n",n->a->name);double best=items[0].n;for(size_t i=1;i<count;i++){if(want_max?items[i].n>best:items[i].n<best)best=items[i].n;}return num(best);}
 if(n->opc==1&&!strcmp(n->a->name,"abs")&&!env_has(e,n->a->name)){if(n->argc!=1||args[0].t!=V_NUM)rt_err("abs expects one number\n");return num(fabs(args[0].n));}
 if(n->opc==1&&!strcmp(n->a->name,"sum")&&!env_has(e,n->a->name)){if(n->argc!=1||args[0].t!=V_LIST)rt_err("sum expects one list\n");double t=0;for(size_t i=0;i<args[0].list->len;i++){if(args[0].list->items[i].t!=V_NUM)rt_err("sum expects a list of numbers\n");t+=args[0].list->items[i].n;}return num(t);}
 if(n->opc==1&&!strcmp(n->a->name,"sorted")&&!env_has(e,n->a->name)){if(n->argc!=1||args[0].t!=V_LIST){rt_err("sorted expects one list\n");exit(3);}Value r=list_value();for(size_t i=0;i<args[0].list->len;i++)list_push(r.list,args[0].list->items[i]);qsort(r.list->items,r.list->len,sizeof(Value),compare_values);return r;}
 if(n->opc==1&&!strcmp(n->a->name,"round")&&!env_has(e,n->a->name)){if(n->argc<1||n->argc>2||args[0].t!=V_NUM){rt_err("round expects a number and optional digits\n");exit(3);}double f=pow(10,n->argc==2?args[1].n:0);return num(round(args[0].n*f)/f);}
 if(n->opc==1&&!strcmp(n->a->name,"bool")&&!env_has(e,n->a->name)){if(n->argc!=1){rt_err("bool expects one value\n");exit(3);}return boolean(truth(args[0]));}
 if(n->opc==1&&!strcmp(n->a->name,"len")&&!env_has(e,n->a->name)){if(n->argc!=1){rt_err("len expects one argument\n");exit(3);}if(args[0].t==V_LIST)return num(args[0].list->len);if(args[0].t==V_STR)return num(text_len(args[0]));if(args[0].t==V_DICT)return num(args[0].dict->len);char*s=stringify(args[0]);return num(strlen(s));}if(n->opc==1&&!strcmp(n->a->name,"str")&&!env_has(e,n->a->name)){if(n->argc!=1){rt_err("str expects one argument\n");exit(3);}return str(stringify(args[0]));}return call(eval(n->a,e),args,n->argc,e);}default:return nul();} }
