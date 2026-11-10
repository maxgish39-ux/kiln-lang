#define KILN_MAX_TRY_DEPTH 4096
static int cur_parse_line=0;static int g_line=0;
static jmp_buf*try_stack[KILN_MAX_TRY_DEPTH];static int try_depth=0;static char err_text[1024];static int err_line=0;
static void raise_text(void){if(try_depth>0)longjmp(*try_stack[try_depth-1],1);fprintf(stderr,"Kiln runtime error (line %d): %s\n",err_line,err_text);exit(3);}
static void rt_err(const char*fmt,...){va_list ap;va_start(ap,fmt);vsnprintf(err_text,sizeof err_text,fmt,ap);va_end(ap);size_t k=strlen(err_text);while(k&&(err_text[k-1]=='\n'))err_text[--k]=0;err_line=g_line;raise_text();}
static Node* node(NType t){Node*n=calloc(1,sizeof(Node));n->t=t;n->line=cur_parse_line;return n;}
static Token*cur(Lexer*l){return &l->v[l->pos];}static bool match(Lexer*l,Tok t){if(cur(l)->t==t){l->pos++;return true;}return false;}static Token take(Lexer*l,Tok t,const char*m){if(cur(l)->t!=t){fprintf(stderr,"Kiln syntax error at %d:%d: expected %s\n",cur(l)->line,cur(l)->col,m);exit(2);}return l->v[l->pos++];}
static Node* expr(Lexer*l);
static Node* block(Lexer*l);
static void fpart(Node*n,Node*part,char*spec){n->args=realloc(n->args,(n->argc+1)*sizeof(Node*));n->params=realloc(n->params,(n->pc+1)*sizeof(char*));n->args[n->argc++]=part;n->params[n->pc++]=spec;}
static void fflush_lit(Node*n,char*lit,size_t*ln){if(!*ln)return;lit[*ln]=0;Node*x=node(N_STR);x->val=str(unescape(lit,*ln));fpart(n,x,NULL);*ln=0;}
static Node* fstring(Token t){Node*n=node(N_FSTR);const char*s=t.s;size_t len=strlen(s);char*lit=calloc(len+1,1);size_t ln=0;
 for(size_t i=0;i<len;){
  if(s[i]=='{'&&i+1<len&&s[i+1]=='{'){lit[ln++]='{';i+=2;continue;}
  if(s[i]=='}'&&i+1<len&&s[i+1]=='}'){lit[ln++]='}';i+=2;continue;}
  if(s[i]=='}'){fprintf(stderr,"Kiln syntax error at %d:%d: single } in f-string (write }} for a brace)\n",t.line,t.col);exit(2);}
  if(s[i]!='{'){lit[ln++]=s[i++];continue;}
  size_t j=i+1;int depth=0;
  while(j<len&&!(s[j]=='}'&&depth==0)){if(s[j]=='{')depth++;else if(s[j]=='}')depth--;j++;}
  if(j>=len){fprintf(stderr,"Kiln syntax error at %d:%d: unclosed { in f-string\n",t.line,t.col);exit(2);}
  fflush_lit(n,lit,&ln);
  size_t cut=j;for(size_t k=i+1;k<j;k++)if(s[k]==':'){cut=k;break;}
  char*code=slice(s+i+1,cut-(i+1));{char*w=code;for(char*r=code;*r;r++){if(*r=='\\'&&r[1]=='"')continue;*w++=*r;}*w=0;}
  char*spec=NULL;
  if(cut<j){spec=slice(s+cut+1,j-cut-1);int ok=spec[0]=='.'&&isdigit((unsigned char)spec[1]);size_t q=2;while(ok&&isdigit((unsigned char)spec[q]))q++;if(ok&&spec[q]=='f')q++;if(!ok||spec[q]!=0||atoi(spec+1)>20){fprintf(stderr,"Kiln syntax error at %d:%d: format spec must look like .2 or .2f\n",t.line,t.col);exit(2);}}
  Lexer sub=lex(code);
  for(int q=0;q<sub.n;q++)sub.v[q].line=t.line;
  Node*e=expr(&sub);
  if(cur(&sub)->t!=T_EOF){fprintf(stderr,"Kiln syntax error at %d:%d: bad expression in f-string\n",t.line,t.col);exit(2);}
  fpart(n,e,spec);
  i=j+1;
 }
 fflush_lit(n,lit,&ln);free(lit);return n;}
static void parse_param_list(Lexer*l,Node*fn){
 Node**defs=NULL;bool any=false;
 if(cur(l)->t!=T_RP){
  do{
   Token t=take(l,T_ID,"parameter");
   Node*d=NULL;
   if(match(l,T_EQ)){d=expr(l);any=true;}
   else if(any){fprintf(stderr,"Kiln syntax error at %d:%d: parameter %s needs a default value, because an earlier parameter has one\n",t.line,t.col,t.s);exit(2);}
   fn->params=realloc(fn->params,(fn->pc+1)*sizeof(char*));fn->params[fn->pc]=t.s;
   defs=realloc(defs,(fn->pc+1)*sizeof(Node*));defs[fn->pc]=d;
   fn->pc++;
  }while(match(l,T_COMMA));
 }
 if(any)fn->args=defs;else free(defs);
}
static Node* primary(Lexer*l){Token t=*cur(l);cur_parse_line=t.line;if(match(l,T_FSTR))return fstring(t);if(match(l,T_NUM)){Node*n=node(N_NUM);n->val=num(t.n);return n;}if(match(l,T_STR)){Node*n=node(N_STR);n->val=str(t.s);return n;}if(match(l,T_TRUE)||match(l,T_FALSE)){Node*n=node(N_BOOL);n->val=boolean(t.t==T_TRUE);return n;}if(match(l,T_NULL))return node(N_NULL);if(match(l,T_ID)){Node*n=node(N_VAR);n->name=t.s;return n;}if(match(l,T_FUN)){Node*n=node(N_LAMBDA);take(l,T_LP,"(");parse_param_list(l,n);take(l,T_RP,")");if(match(l,T_ARROW)){Node*blk=node(N_BLOCK);Node*ret=node(N_RET);ret->a=expr(l);blk->next=ret;n->a=blk;}else n->a=block(l);return n;}
if(match(l,T_LS)){Node*n=node(N_LIST);if(cur(l)->t!=T_RS){do{n->args=realloc(n->args,(n->argc+1)*sizeof(Node*));n->args[n->argc++]=expr(l);}while(match(l,T_COMMA));}take(l,T_RS,"]");return n;}
if(match(l,T_LB)){Node*n=node(N_DICT);if(cur(l)->t!=T_RB){do{Node*k=expr(l);take(l,T_COLON,":");Node*v=expr(l);n->args=realloc(n->args,(n->argc+2)*sizeof(Node*));n->args[n->argc++]=k;n->args[n->argc++]=v;}while(match(l,T_COMMA));}take(l,T_RB,"}");return n;}
if(match(l,T_LP)){Node*n=expr(l);take(l,T_RP,")");return n;}fprintf(stderr,"Kiln syntax error at %d:%d: expected expression\n",t.line,t.col);exit(2);}
enum { OP_AND,OP_OR,OP_IN,OP_ADD,OP_SUB,OP_MUL,OP_DIV,OP_FLOORDIV,OP_MOD,OP_EQ,OP_NE,OP_LT,OP_GT,OP_LE,OP_GE,OP_POW,OP_NOTIN,OP_UNKNOWN };
static int op_code(const char*s){static const char*names[]={"and","or","in","+","-","*","/","//","%","==","!=","<",">","<=",">=","**"};for(int i=0;i<16;i++)if(!strcmp(names[i],s))return i;return OP_UNKNOWN;}
static bool is_global_builtin(const char*s){static const char*names[]={"range","input","type","int","float","sorted","round","bool","len","str","assert","enumerate","zip","min","max","abs","sum",NULL};for(int i=0;names[i];i++)if(!strcmp(names[i],s))return true;return false;}
static Node* bin(Lexer*l,int level);
static void add_arg(Node*n,Node*x){n->args=realloc(n->args,(n->argc+1)*sizeof(Node*));n->args[n->argc++]=x;}
static void parse_unpack_rhs(Lexer*l,Node*u){
 Node*first=expr(l);
 if(cur(l)->t==T_COMMA){Node*list=node(N_LIST);add_arg(list,first);while(match(l,T_COMMA))add_arg(list,expr(l));u->c=list;}
 else u->a=first;}
static Node* postfix(Lexer*l){Node*n=primary(l);for(;;){if(match(l,T_LP)){Node*c=node(N_CALL);c->a=n;c->opc=(n->t==N_VAR&&is_global_builtin(n->name))?1:2;if(cur(l)->t!=T_RP){do{c->args=realloc(c->args,(c->argc+1)*sizeof(Node*));c->args[c->argc++]=expr(l);}while(match(l,T_COMMA));}take(l,T_RP,")");n=c;}else if(match(l,T_DOT)){Token id=take(l,T_ID,"module member");Node*c=node(N_EXPR);c->a=n;c->name=id.s;n=c;}else if(match(l,T_LS)){Node*c;Node*first=NULL;if(cur(l)->t!=T_COLON)first=expr(l);if(match(l,T_COLON)){c=node(N_SLICE);c->a=n;c->b=first;if(cur(l)->t!=T_RS)c->c=expr(l);}else{c=node(N_INDEX);c->a=n;c->b=first;}take(l,T_RS,"]");n=c;}else break;}return n;}
static Node* unary(Lexer*l){if(match(l,T_NOT)){Node*n=node(N_UN);n->op="not";n->a=unary(l);return n;}if(match(l,T_MINUS)){Node*n=node(N_UN);n->op="-";n->a=bin(l,6);return n;}if(match(l,T_PLUS)){Node*n=node(N_UN);n->op="+";n->a=unary(l);return n;}return postfix(l);}
static Node* bin(Lexer*l,int level){static Tok ops[7][5]={{T_OR,0,0,0,0},{T_AND,0,0,0,0},{T_EQEQ,T_NE,0,0,0},{T_LT,T_GT,T_LE,T_GE,T_IN},{T_PLUS,T_MINUS,0,0,0},{T_STAR,T_SLASH,T_PERCENT,T_SLASHSLASH,0},{T_POW,0,0,0,0}};static const char*names[7][5]={{"or"},{"and"},{"==","!="},{"<",">","<=",">=","in"},{"+","-"},{"*","/","%","//"},{"**"}};if(level==7)return unary(l);Node*n=bin(l,level+1);for(;;){if(level==3&&cur(l)->t==T_NOT&&l->v[l->pos+1].t==T_IN){l->pos+=2;Node*x=node(N_BIN);x->op="not in";x->opc=OP_NOTIN;x->a=n;x->b=bin(l,level+1);n=x;continue;}int k=-1;for(int j=0;j<5&&ops[level][j];j++)if(cur(l)->t==ops[level][j])k=j;if(k<0)break;l->pos++;Node*x=node(N_BIN);x->op=(char*)names[level][k];x->opc=op_code(names[level][k]);x->a=n;x->b=bin(l,level==6?level:level+1);n=x;}return n;}
static Node* expr(Lexer*l){
 Node*v=bin(l,0);
 if(cur(l)->t==T_IF){
  int save=l->pos;l->pos++;
  Node*c=bin(l,0);
  if(cur(l)->t==T_ELSE){l->pos++;Node*alt=expr(l);Node*t=node(N_COND);t->a=c;t->b=v;t->c=alt;return t;}
  l->pos=save;
 }
 return v;}
static Node* block(Lexer*l);
static Node* assign_node(Node*x,Node*value){
 Node*n;
 if(x->t==N_VAR){n=node(N_SET);n->name=x->name;n->a=value;}
 else if(x->t==N_INDEX){n=node(N_INDEX_SET);n->a=x->a;n->b=x->b;n->c=value;}
 else if(x->t==N_EXPR){Node*k=node(N_STR);k->val=str(x->name);n=node(N_INDEX_SET);n->a=x->a;n->b=k;n->c=value;}
 else{fprintf(stderr,"Kiln syntax error: invalid assignment target\n");exit(2);}
 return n;}
static Node* stmt(Lexer*l){Token t=*cur(l);cur_parse_line=t.line;if(match(l,T_LET)){Node*first=node(N_VAR);first->name=take(l,T_ID,"variable name").s;if(cur(l)->t==T_COMMA){Node*u=node(N_UNPACK);u->opc=1;add_arg(u,first);while(match(l,T_COMMA)){Node*v=node(N_VAR);v->name=take(l,T_ID,"variable name").s;add_arg(u,v);}take(l,T_EQ,"=");parse_unpack_rhs(l,u);match(l,T_SEMI);return u;}Node*n=node(N_LET);n->name=first->name;take(l,T_EQ,"=");n->a=expr(l);match(l,T_SEMI);return n;}if(match(l,T_PRINT)){Node*n=node(N_PRINT);take(l,T_LP,"(");if(cur(l)->t!=T_RP){do{n->args=realloc(n->args,(n->argc+1)*sizeof(Node*));n->args[n->argc++]=expr(l);}while(match(l,T_COMMA));}take(l,T_RP,")");match(l,T_SEMI);return n;}if(match(l,T_IF)){Node*n=node(N_IF);n->a=expr(l);n->b=block(l);if(match(l,T_ELSE)){if(cur(l)->t==T_IF){Node*blk=node(N_BLOCK);blk->next=stmt(l);n->c=blk;}else n->c=block(l);}return n;}if(match(l,T_WHILE)){Node*n=node(N_WHILE);n->a=expr(l);n->b=block(l);return n;}if(match(l,T_FOR)){Node*n=node(N_FOR);take(l,T_LP,"(");take(l,T_LET,"let");n->name=take(l,T_ID,"loop variable").s;take(l,T_IN,"in");n->a=expr(l);take(l,T_RP,")");n->b=block(l);return n;}if(match(l,T_BREAK)){Node*n=node(N_BREAK);match(l,T_SEMI);return n;}if(match(l,T_CONTINUE)){Node*n=node(N_CONTINUE);match(l,T_SEMI);return n;}if(cur(l)->t==T_FUN&&l->v[l->pos+1].t!=T_LP&&match(l,T_FUN)){Node*n=node(N_FUN);n->name=take(l,T_ID,"function name").s;take(l,T_LP,"(");parse_param_list(l,n);take(l,T_RP,")");n->a=block(l);return n;}if(match(l,T_TRY)){Node*n=node(N_TRY);n->a=block(l);if(match(l,T_CATCH)){if(match(l,T_LP)){n->name=take(l,T_ID,"error variable name").s;take(l,T_RP,")");}n->b=block(l);}if(match(l,T_FINALLY))n->c=block(l);if(!n->b&&!n->c){fprintf(stderr,"Kiln syntax error at %d:%d: try needs catch or finally\n",cur(l)->line,cur(l)->col);exit(2);}return n;}if(match(l,T_THROW)){Node*n=node(N_THROW);n->a=expr(l);match(l,T_SEMI);return n;}if(match(l,T_CLASS)){Node*n=node(N_CLASS);n->name=take(l,T_ID,"class name").s;take(l,T_LB,"{");while(!match(l,T_RB)){if(cur(l)->t==T_EOF){fprintf(stderr,"Kiln syntax error at %d:%d: expected }\n",cur(l)->line,cur(l)->col);exit(2);}take(l,T_FUN,"method definition");Node*m=node(N_FUN);m->name=take(l,T_ID,"method name").s;take(l,T_LP,"(");parse_param_list(l,m);take(l,T_RP,")");m->a=block(l);n->args=realloc(n->args,(n->argc+1)*sizeof(Node*));n->args[n->argc++]=m;}return n;}if(match(l,T_RETURN)){Node*n=node(N_RET);if(cur(l)->t!=T_SEMI&&cur(l)->t!=T_RB)n->a=expr(l);match(l,T_SEMI);return n;}if(match(l,T_IMPORT)){Node*n=node(N_IMPORT);n->name=take(l,T_STR,"module name").s;take(l,T_AS,"as");n->op=take(l,T_ID,"module alias").s;match(l,T_SEMI);return n;}if(cur(l)->t==T_LB)return block(l);Node*x=expr(l);
 if(cur(l)->t==T_COMMA){Node*u=node(N_UNPACK);u->opc=0;add_arg(u,x);while(match(l,T_COMMA))add_arg(u,expr(l));take(l,T_EQ,"=");parse_unpack_rhs(l,u);match(l,T_SEMI);return u;}
 {Tok cop=cur(l)->t;int bop=-1;const char*bname=NULL;
  if(cop==T_PLUSEQ){bop=OP_ADD;bname="+";}else if(cop==T_MINUSEQ){bop=OP_SUB;bname="-";}else if(cop==T_STAREQ){bop=OP_MUL;bname="*";}else if(cop==T_SLASHEQ){bop=OP_DIV;bname="/";}else if(cop==T_PERCENTEQ){bop=OP_MOD;bname="%";}
  if(bop>=0){l->pos++;Node*rhs=expr(l);Node*b=node(N_BIN);b->op=(char*)bname;b->opc=bop;b->a=x;b->b=rhs;Node*n=assign_node(x,b);match(l,T_SEMI);return n;}}
 if(match(l,T_EQ)){Node*n=assign_node(x,expr(l));match(l,T_SEMI);return n;}Node*n=node(N_EXPR);n->a=x;match(l,T_SEMI);(void)t;return n;}
static Node* block(Lexer*l){take(l,T_LB,"{");Node*n=node(N_BLOCK),**tail=&n->next;while(cur(l)->t!=T_RB&&cur(l)->t!=T_EOF){Node*x=stmt(l);*tail=x;tail=&x->next;}take(l,T_RB,"}");return n;}
static Node* parse(Lexer*l){Node*n=node(N_BLOCK),**tail=&n->next;while(cur(l)->t!=T_EOF){Node*x=stmt(l);*tail=x;tail=&x->next;}return n;}
