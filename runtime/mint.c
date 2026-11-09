/* MINT native runtime: a compact C implementation of the language core.
 * Build: cc -std=c11 -O2 runtime/mint.c -lm -o mint
 * This runtime deliberately has no Python dependency.
 */
#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include <stdarg.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef MINT_WITH_SQLITE
#include <sqlite3.h>
#endif
#ifdef MINT_WITH_RAYLIB
#include <raylib.h>
#endif

typedef enum { T_EOF,T_NUM,T_STR,T_ID,T_LET,T_FUN,T_RETURN,T_PRINT,T_IF,T_ELSE,T_WHILE,T_TRUE,T_FALSE,T_NULL,T_AND,T_OR,T_NOT,T_IMPORT,T_AS,T_FOR,T_IN,T_BREAK,T_CONTINUE,T_LP,T_RP,T_LB,T_RB,T_LS,T_RS,T_COMMA,T_COLON,T_DOT,T_SEMI,T_PLUS,T_MINUS,T_STAR,T_SLASH,T_PERCENT,T_EQ,T_EQEQ,T_NE,T_LT,T_GT,T_LE,T_GE,T_ARROW,T_CLASS,T_TRY,T_CATCH,T_THROW,T_FSTR } Tok;
typedef struct { Tok t; char *s; double n; int line,col; } Token;
typedef struct { Token *v; int n,cap,pos; } Lexer;
static char *slice(const char*s,size_t n){char*r=malloc(n+1);memcpy(r,s,n);r[n]=0;return r;}
static void push(Lexer*l,Token t){if(l->n==l->cap){l->cap=l->cap?l->cap*2:128;l->v=realloc(l->v,l->cap*sizeof(Token));}l->v[l->n++]=t;}
static void lexerr(int line,int col,const char*m){fprintf(stderr,"MINT syntax error at %d:%d: %s\n",line,col,m);exit(2);}
static Tok keyword(const char*s){struct{const char*s;Tok t;}k[]={{"let",T_LET},{"fun",T_FUN},{"return",T_RETURN},{"print",T_PRINT},{"if",T_IF},{"else",T_ELSE},{"while",T_WHILE},{"true",T_TRUE},{"false",T_FALSE},{"null",T_NULL},{"and",T_AND},{"or",T_OR},{"not",T_NOT},{"import",T_IMPORT},{"as",T_AS},{"for",T_FOR},{"in",T_IN},{"break",T_BREAK},{"continue",T_CONTINUE},{"class",T_CLASS},{"try",T_TRY},{"catch",T_CATCH},{"throw",T_THROW}};for(size_t i=0;i<sizeof(k)/sizeof(*k);i++)if(!strcmp(s,k[i].s))return k[i].t;return T_ID;}
static char* unescape(const char*s,size_t n){char*out=malloc(n+1);size_t o=0;for(size_t i=0;i<n;i++){if(s[i]=='\\'&&i+1<n){i++;switch(s[i]){case 'n':out[o++]='\n';break;case 't':out[o++]='\t';break;case 'r':out[o++]='\r';break;case '0':out[o++]='\0';break;case '\\':out[o++]='\\';break;case '"':out[o++]='"';break;default:out[o++]='\\';out[o++]=s[i];}}else out[o++]=s[i];}out[o]=0;return out;}
static Lexer lex(const char*src){Lexer l={0};int line=1,col=1;for(size_t i=0;src[i];){char c=src[i];if(c==' '||c=='\t'||c=='\r'){i++;col++;continue;}if(c=='\n'){i++;line++;col=1;continue;}if(c=='#'){while(src[i]&&src[i]!='\n'){i++;col++;}continue;}Token x={.line=line,.col=col};size_t st=i;if(isdigit((unsigned char)c)){while(isdigit((unsigned char)src[i])){i++;col++;}if(src[i]=='.'&&isdigit((unsigned char)src[i+1])){i++;col++;while(isdigit((unsigned char)src[i])){i++;col++;}}x.t=T_NUM;x.s=slice(src+st,i-st);x.n=strtod(x.s,NULL);push(&l,x);continue;}if(c=='f'&&src[i+1]=='"'){i+=2;col+=2;st=i;while(src[i]&&src[i]!='"'){if(src[i]=='\\'&&src[i+1])i++;if(src[i]=='\n'){line++;col=0;}i++;col++;}if(!src[i])lexerr(line,col,"unterminated f-string");x.s=slice(src+st,i-st);i++;col++;x.t=T_FSTR;push(&l,x);continue;}if(isalpha((unsigned char)c)||c=='_'){while(isalnum((unsigned char)src[i])||src[i]=='_'){i++;col++;}x.s=slice(src+st,i-st);x.t=keyword(x.s);push(&l,x);continue;}if(c=='"'){i++;col++;st=i;while(src[i]&&src[i]!='"'){if(src[i]=='\\'&&src[i+1])i++;if(src[i]=='\n'){line++;col=0;}i++;col++;}if(!src[i])lexerr(line,col,"unterminated string");x.s=unescape(src+st,i-st);i++;col++;x.t=T_STR;push(&l,x);continue;}i++;col++;switch(c){case '(':x.t=T_LP;break;case ')':x.t=T_RP;break;case '{':x.t=T_LB;break;case '}':x.t=T_RB;break;case '[':x.t=T_LS;break;case ']':x.t=T_RS;break;case ',':x.t=T_COMMA;break;case ':':x.t=T_COLON;break;case '.':x.t=T_DOT;break;case ';':x.t=T_SEMI;break;case '+':x.t=T_PLUS;break;case '-':x.t=T_MINUS;break;case '*':x.t=T_STAR;break;case '/':x.t=T_SLASH;break;case '%':x.t=T_PERCENT;break;case '=':if(src[i]=='='){i++;col++;x.t=T_EQEQ;}else if(src[i]=='>'){i++;col++;x.t=T_ARROW;}else x.t=T_EQ;break;case '!':if(src[i++]=='='){col++;x.t=T_NE;}else lexerr(line,col,"expected !=");break;case '<':if(src[i]=='='){i++;col++;x.t=T_LE;}else x.t=T_LT;break;case '>':if(src[i]=='='){i++;col++;x.t=T_GE;}else x.t=T_GT;break;default:lexerr(line,col,"unexpected character");}x.s=slice(src+st,i-st);push(&l,x);}push(&l,(Token){.t=T_EOF,.line=line,.col=col});return l;}

typedef enum{V_NULL,V_NUM,V_BOOL,V_STR,V_FUNC,V_MOD,V_DB,V_LIST,V_DICT,V_INST,V_CLASS} VType;
typedef struct Value Value;typedef struct Node Node;typedef struct Env Env;
typedef struct {char**names;int count;Node*body;Env*closure;} Func;
typedef struct {Value *items;size_t len,cap;} List;
typedef struct {char **keys;Value *values;size_t len,cap;} Dict;
struct Value{VType t;double n;bool b;char*s;Func*f;void*ptr;List*list;Dict*dict;};
static Value nul(void){return(Value){.t=V_NULL};}static Value num(double n){return(Value){.t=V_NUM,.n=n};}static Value boolean(bool b){return(Value){.t=V_BOOL,.b=b};}static Value str(const char*s){return(Value){.t=V_STR,.s=strdup(s?s:"")};}
typedef struct Class Class;typedef struct Instance Instance;
struct Class{char*name;Node**methods;int mcount;Env*env;};
struct Instance{Class*cls;Value fields;};
static char* stringify(Value v){char b[96];switch(v.t){case V_NULL:return strdup("null");case V_BOOL:return strdup(v.b?"true":"false");case V_NUM:snprintf(b,sizeof b,"%.15g",v.n);return strdup(b);case V_STR:return strdup(v.s);case V_FUNC:return strdup("<function>");case V_MOD:return strdup("<module>");case V_DB:return strdup("<database>");case V_INST:{char*out=malloc(strlen(((Instance*)v.ptr)->cls->name)+16);sprintf(out,"<%s instance>",((Instance*)v.ptr)->cls->name);return out;}case V_CLASS:{char*out=malloc(strlen(((Class*)v.ptr)->name)+16);sprintf(out,"<class %s>",((Class*)v.ptr)->name);return out;}case V_LIST:{size_t cap=32;char*out=calloc(cap,1);strcat(out,"[");for(size_t i=0;i<v.list->len;i++){char*x=stringify(v.list->items[i]);size_t need=strlen(out)+strlen(x)+4;if(need>cap){cap=need*2;out=realloc(out,cap);}if(i)strcat(out,", ");strcat(out,x);}strcat(out,"]");return out;}case V_DICT:{size_t cap=32;char*out=calloc(cap,1);strcat(out,"{");for(size_t i=0;i<v.dict->len;i++){char*x=stringify(str(v.dict->keys[i]));char*y=stringify(v.dict->values[i]);size_t need=strlen(out)+strlen(x)+strlen(y)+6;if(need>cap){cap=need*2;out=realloc(out,cap);}if(i)strcat(out,", ");strcat(out,x);strcat(out,": ");strcat(out,y);}strcat(out,"}");return out;}}return strdup("?");}
static bool truth(Value v){return v.t!=V_NULL&&(v.t!=V_BOOL||v.b)&&(v.t!=V_NUM||v.n!=0)&&(v.t!=V_STR||v.s[0])&&(v.t!=V_LIST||v.list->len>0)&&(v.t!=V_DICT||v.dict->len>0);}
typedef enum{N_NUM,N_STR,N_BOOL,N_NULL,N_VAR,N_BIN,N_UN,N_LET,N_SET,N_PRINT,N_IF,N_WHILE,N_BLOCK,N_FUN,N_CALL,N_RET,N_IMPORT,N_EXPR,N_FOR,N_BREAK,N_CONTINUE,N_LIST,N_DICT,N_INDEX,N_INDEX_SET,N_LAMBDA,N_CLASS,N_TRY,N_THROW,N_FSTR} NType;
struct Node{NType t;char*name;char*op;Value val;Node*a,*b,*c,*next;Node**args;int argc;char**params;int pc;int line;};
static int cur_parse_line=0;static int g_line=0;
static jmp_buf*try_stack[256];static int try_depth=0;static char err_text[1024];static int err_line=0;
static void raise_text(void){if(try_depth>0)longjmp(*try_stack[try_depth-1],1);fprintf(stderr,"MINT runtime error (line %d): %s\n",err_line,err_text);exit(3);}
static void rt_err(const char*fmt,...){va_list ap;va_start(ap,fmt);vsnprintf(err_text,sizeof err_text,fmt,ap);va_end(ap);size_t k=strlen(err_text);while(k&&(err_text[k-1]=='\n'))err_text[--k]=0;err_line=g_line;raise_text();}
static Node* node(NType t){Node*n=calloc(1,sizeof(Node));n->t=t;n->line=cur_parse_line;return n;}
static Token*cur(Lexer*l){return &l->v[l->pos];}static bool match(Lexer*l,Tok t){if(cur(l)->t==t){l->pos++;return true;}return false;}static Token take(Lexer*l,Tok t,const char*m){if(cur(l)->t!=t){fprintf(stderr,"MINT syntax error at %d:%d: expected %s\n",cur(l)->line,cur(l)->col,m);exit(2);}return l->v[l->pos++];}
static Node* expr(Lexer*l);
static Node* block(Lexer*l);
static void fpart(Node*n,Node*part,char*spec){n->args=realloc(n->args,(n->argc+1)*sizeof(Node*));n->params=realloc(n->params,(n->pc+1)*sizeof(char*));n->args[n->argc++]=part;n->params[n->pc++]=spec;}
static void fflush_lit(Node*n,char*lit,size_t*ln){if(!*ln)return;lit[*ln]=0;Node*x=node(N_STR);x->val=str(unescape(lit,*ln));fpart(n,x,NULL);*ln=0;}
static Node* fstring(Token t){Node*n=node(N_FSTR);const char*s=t.s;size_t len=strlen(s);char*lit=calloc(len+1,1);size_t ln=0;
 for(size_t i=0;i<len;){
  if(s[i]=='{'&&i+1<len&&s[i+1]=='{'){lit[ln++]='{';i+=2;continue;}
  if(s[i]=='}'&&i+1<len&&s[i+1]=='}'){lit[ln++]='}';i+=2;continue;}
  if(s[i]=='}'){fprintf(stderr,"MINT syntax error at %d:%d: single } in f-string (write }} for a brace)\n",t.line,t.col);exit(2);}
  if(s[i]!='{'){lit[ln++]=s[i++];continue;}
  size_t j=i+1;int depth=0;
  while(j<len&&!(s[j]=='}'&&depth==0)){if(s[j]=='{')depth++;else if(s[j]=='}')depth--;j++;}
  if(j>=len){fprintf(stderr,"MINT syntax error at %d:%d: unclosed { in f-string\n",t.line,t.col);exit(2);}
  fflush_lit(n,lit,&ln);
  size_t cut=j;for(size_t k=i+1;k<j;k++)if(s[k]==':'){cut=k;break;}
  char*code=slice(s+i+1,cut-(i+1));{char*w=code;for(char*r=code;*r;r++){if(*r=='\\'&&r[1]=='"')continue;*w++=*r;}*w=0;}
  char*spec=NULL;
  if(cut<j){spec=slice(s+cut+1,j-cut-1);int ok=spec[0]=='.'&&isdigit((unsigned char)spec[1]);size_t q=2;while(ok&&isdigit((unsigned char)spec[q]))q++;if(ok&&spec[q]=='f')q++;if(!ok||spec[q]!=0||atoi(spec+1)>20){fprintf(stderr,"MINT syntax error at %d:%d: format spec must look like .2 or .2f\n",t.line,t.col);exit(2);}}
  Lexer sub=lex(code);
  for(int q=0;q<sub.n;q++)sub.v[q].line=t.line;
  Node*e=expr(&sub);
  if(cur(&sub)->t!=T_EOF){fprintf(stderr,"MINT syntax error at %d:%d: bad expression in f-string\n",t.line,t.col);exit(2);}
  fpart(n,e,spec);
  i=j+1;
 }
 fflush_lit(n,lit,&ln);free(lit);return n;}
static Node* primary(Lexer*l){Token t=*cur(l);cur_parse_line=t.line;if(match(l,T_FSTR))return fstring(t);if(match(l,T_NUM)){Node*n=node(N_NUM);n->val=num(t.n);return n;}if(match(l,T_STR)){Node*n=node(N_STR);n->val=str(t.s);return n;}if(match(l,T_TRUE)||match(l,T_FALSE)){Node*n=node(N_BOOL);n->val=boolean(t.t==T_TRUE);return n;}if(match(l,T_NULL))return node(N_NULL);if(match(l,T_ID)){Node*n=node(N_VAR);n->name=t.s;return n;}if(match(l,T_FUN)){Node*n=node(N_LAMBDA);take(l,T_LP,"(");if(cur(l)->t!=T_RP){do{n->params=realloc(n->params,(n->pc+1)*sizeof(char*));n->params[n->pc++]=take(l,T_ID,"parameter").s;}while(match(l,T_COMMA));}take(l,T_RP,")");if(match(l,T_ARROW)){Node*blk=node(N_BLOCK);Node*ret=node(N_RET);ret->a=expr(l);blk->next=ret;n->a=blk;}else n->a=block(l);return n;}
if(match(l,T_LS)){Node*n=node(N_LIST);if(cur(l)->t!=T_RS){do{n->args=realloc(n->args,(n->argc+1)*sizeof(Node*));n->args[n->argc++]=expr(l);}while(match(l,T_COMMA));}take(l,T_RS,"]");return n;}
if(match(l,T_LB)){Node*n=node(N_DICT);if(cur(l)->t!=T_RB){do{Node*k=expr(l);take(l,T_COLON,":");Node*v=expr(l);n->args=realloc(n->args,(n->argc+2)*sizeof(Node*));n->args[n->argc++]=k;n->args[n->argc++]=v;}while(match(l,T_COMMA));}take(l,T_RB,"}");return n;}
if(match(l,T_LP)){Node*n=expr(l);take(l,T_RP,")");return n;}fprintf(stderr,"MINT syntax error at %d:%d: expected expression\n",t.line,t.col);exit(2);}
static Node* postfix(Lexer*l){Node*n=primary(l);for(;;){if(match(l,T_LP)){Node*c=node(N_CALL);c->a=n;if(cur(l)->t!=T_RP){do{c->args=realloc(c->args,(c->argc+1)*sizeof(Node*));c->args[c->argc++]=expr(l);}while(match(l,T_COMMA));}take(l,T_RP,")");n=c;}else if(match(l,T_DOT)){Token id=take(l,T_ID,"module member");Node*c=node(N_EXPR);c->a=n;c->name=id.s;n=c;}else if(match(l,T_LS)){Node*c=node(N_INDEX);c->a=n;c->b=expr(l);take(l,T_RS,"]");n=c;}else break;}return n;}
static Node* unary(Lexer*l){if(match(l,T_NOT)){Node*n=node(N_UN);n->op="not";n->a=unary(l);return n;}if(match(l,T_MINUS)){Node*n=node(N_UN);n->op="-";n->a=unary(l);return n;}if(match(l,T_PLUS)){Node*n=node(N_UN);n->op="+";n->a=unary(l);return n;}return postfix(l);}
static Node* bin(Lexer*l,int level){static Tok ops[6][5]={{T_OR,0,0,0,0},{T_AND,0,0,0,0},{T_EQEQ,T_NE,0,0,0},{T_LT,T_GT,T_LE,T_GE,T_IN},{T_PLUS,T_MINUS,0,0,0},{T_STAR,T_SLASH,T_PERCENT,0,0}};static const char*names[6][5]={{"or"},{"and"},{"==","!="},{"<",">","<=",">=","in"},{"+","-"},{"*","/","%"}};if(level==6)return unary(l);Node*n=bin(l,level+1);for(;;){int k=-1;for(int j=0;j<5&&ops[level][j];j++)if(cur(l)->t==ops[level][j])k=j;if(k<0)break;l->pos++;Node*x=node(N_BIN);x->op=(char*)names[level][k];x->a=n;x->b=bin(l,level+1);n=x;}return n;}
static Node* expr(Lexer*l){return bin(l,0);}
static Node* block(Lexer*l);
static Node* stmt(Lexer*l){Token t=*cur(l);cur_parse_line=t.line;if(match(l,T_LET)){Node*n=node(N_LET);n->name=take(l,T_ID,"variable name").s;take(l,T_EQ,"=");n->a=expr(l);match(l,T_SEMI);return n;}if(match(l,T_PRINT)){Node*n=node(N_PRINT);take(l,T_LP,"(");n->a=expr(l);take(l,T_RP,")");match(l,T_SEMI);return n;}if(match(l,T_IF)){Node*n=node(N_IF);n->a=expr(l);n->b=block(l);if(match(l,T_ELSE)){if(cur(l)->t==T_IF){Node*blk=node(N_BLOCK);blk->next=stmt(l);n->c=blk;}else n->c=block(l);}return n;}if(match(l,T_WHILE)){Node*n=node(N_WHILE);n->a=expr(l);n->b=block(l);return n;}if(match(l,T_FOR)){Node*n=node(N_FOR);take(l,T_LP,"(");take(l,T_LET,"let");n->name=take(l,T_ID,"loop variable").s;take(l,T_IN,"in");n->a=expr(l);take(l,T_RP,")");n->b=block(l);return n;}if(match(l,T_BREAK)){Node*n=node(N_BREAK);match(l,T_SEMI);return n;}if(match(l,T_CONTINUE)){Node*n=node(N_CONTINUE);match(l,T_SEMI);return n;}if(cur(l)->t==T_FUN&&l->v[l->pos+1].t!=T_LP&&match(l,T_FUN)){Node*n=node(N_FUN);n->name=take(l,T_ID,"function name").s;take(l,T_LP,"(");if(cur(l)->t!=T_RP){do{n->params=realloc(n->params,(n->pc+1)*sizeof(char*));n->params[n->pc++]=take(l,T_ID,"parameter").s;}while(match(l,T_COMMA));}take(l,T_RP,")");n->a=block(l);return n;}if(match(l,T_TRY)){Node*n=node(N_TRY);n->a=block(l);take(l,T_CATCH,"catch");if(match(l,T_LP)){n->name=take(l,T_ID,"error variable name").s;take(l,T_RP,")");}n->b=block(l);return n;}if(match(l,T_THROW)){Node*n=node(N_THROW);n->a=expr(l);match(l,T_SEMI);return n;}if(match(l,T_CLASS)){Node*n=node(N_CLASS);n->name=take(l,T_ID,"class name").s;take(l,T_LB,"{");while(!match(l,T_RB)){if(cur(l)->t==T_EOF){fprintf(stderr,"MINT syntax error at %d:%d: expected }\n",cur(l)->line,cur(l)->col);exit(2);}take(l,T_FUN,"method definition");Node*m=node(N_FUN);m->name=take(l,T_ID,"method name").s;take(l,T_LP,"(");if(cur(l)->t!=T_RP){do{m->params=realloc(m->params,(m->pc+1)*sizeof(char*));m->params[m->pc++]=take(l,T_ID,"parameter").s;}while(match(l,T_COMMA));}take(l,T_RP,")");m->a=block(l);n->args=realloc(n->args,(n->argc+1)*sizeof(Node*));n->args[n->argc++]=m;}return n;}if(match(l,T_RETURN)){Node*n=node(N_RET);if(cur(l)->t!=T_SEMI&&cur(l)->t!=T_RB)n->a=expr(l);match(l,T_SEMI);return n;}if(match(l,T_IMPORT)){Node*n=node(N_IMPORT);n->name=take(l,T_STR,"module name").s;take(l,T_AS,"as");n->op=take(l,T_ID,"module alias").s;match(l,T_SEMI);return n;}if(cur(l)->t==T_LB)return block(l);Node*x=expr(l);if(match(l,T_EQ)){Node*n;if(x->t==N_VAR){n=node(N_SET);n->name=x->name;n->a=expr(l);}else if(x->t==N_INDEX){n=node(N_INDEX_SET);n->a=x->a;n->b=x->b;n->c=expr(l);}else if(x->t==N_EXPR){Node*k=node(N_STR);k->val=str(x->name);n=node(N_INDEX_SET);n->a=x->a;n->b=k;n->c=expr(l);}else{fprintf(stderr,"MINT syntax error: invalid assignment target\n");exit(2);}match(l,T_SEMI);return n;}Node*n=node(N_EXPR);n->a=x;match(l,T_SEMI);(void)t;return n;}
static Node* block(Lexer*l){take(l,T_LB,"{");Node*n=node(N_BLOCK),**tail=&n->next;while(cur(l)->t!=T_RB&&cur(l)->t!=T_EOF){Node*x=stmt(l);*tail=x;tail=&x->next;}take(l,T_RB,"}");return n;}
static Node* parse(Lexer*l){Node*n=node(N_BLOCK),**tail=&n->next;while(cur(l)->t!=T_EOF){Node*x=stmt(l);*tail=x;tail=&x->next;}return n;}
struct Env{char**keys;Value*vals;int n,cap;Env*parent;};static Env* env_new(Env*p){Env*e=calloc(1,sizeof(Env));e->parent=p;return e;}static int find(Env*e,const char*k){for(int i=0;i<e->n;i++)if(!strcmp(e->keys[i],k))return i;return -1;}static void define(Env*e,const char*k,Value v){int i=find(e,k);if(i>=0){e->vals[i]=v;return;}if(e->n==e->cap){e->cap=e->cap?e->cap*2:16;e->keys=realloc(e->keys,e->cap*sizeof(char*));e->vals=realloc(e->vals,e->cap*sizeof(Value));}e->keys[e->n]=strdup(k);e->vals[e->n++]=v;}static Value get(Env*e,const char*k){for(;e;e=e->parent){int i=find(e,k);if(i>=0)return e->vals[i];}rt_err("undefined name '%s'\n",k);exit(3);}static void set(Env*e,const char*k,Value v){for(;e;e=e->parent){int i=find(e,k);if(i>=0){e->vals[i]=v;return;}}rt_err("undefined name '%s'\n",k);exit(3);}
static bool returning=false;static Value retvalue;static int loop_signal=0;static int loop_depth=0;static int loop_floor=0;
static Value eval(Node*n,Env*e);
static Value list_value(void){Value v={.t=V_LIST,.list=calloc(1,sizeof(List))};return v;}
static Value dict_value(void){Value v={.t=V_DICT,.dict=calloc(1,sizeof(Dict))};return v;}
static void list_push(List*l,Value v){if(l->len==l->cap){l->cap=l->cap?l->cap*2:8;l->items=realloc(l->items,l->cap*sizeof(Value));}l->items[l->len++]=v;}
static long list_index(Value idx,size_t len){if(idx.t!=V_NUM||floor(idx.n)!=idx.n){rt_err("list index must be an integer\n");exit(3);}long i=(long)idx.n;if(i<0)i+=(long)len;if(i<0||(size_t)i>=len){rt_err("list index out of range\n");exit(3);}return i;}
static long dict_find(Dict*d,const char*k){for(size_t i=0;i<d->len;i++)if(!strcmp(d->keys[i],k))return (long)i;return -1;}
static void dict_set(Dict*d,const char*k,Value v){long i=dict_find(d,k);if(i>=0){d->values[i]=v;return;}if(d->len==d->cap){d->cap=d->cap?d->cap*2:8;d->keys=realloc(d->keys,d->cap*sizeof(char*));d->values=realloc(d->values,d->cap*sizeof(Value));}d->keys[d->len]=strdup(k);d->values[d->len++]=v;}

static Node* find_method(Class*c,const char*name){for(int i=0;i<c->mcount;i++)if(!strcmp(c->methods[i]->name,name))return c->methods[i];return NULL;}
static Value bind_method(Class*c,Node*m,Value self){Env*menv=env_new(c->env);define(menv,"this",self);Func*f=calloc(1,sizeof(Func));f->names=m->params;f->count=m->pc;f->body=m->a;f->closure=menv;Value v={.t=V_FUNC,.f=f};return v;}
static Value instance_new(Class*c){Instance*in=calloc(1,sizeof(Instance));in->cls=c;in->fields=dict_value();Value v={.t=V_INST,.ptr=in};return v;}
static Value instance_member(Value self,const char*name){Instance*in=self.ptr;long i=dict_find(in->fields.dict,name);if(i>=0)return in->fields.dict->values[i];Node*m=find_method(in->cls,name);if(m)return bind_method(in->cls,m,self);rt_err("instance of %s has no member '%s'\n",in->cls->name,name);return nul();}
static Value call(Value fn,Value*args,int argc,Env*e){
 if(fn.t==V_CLASS){Class*c=fn.ptr;Value self=instance_new(c);Node*init=find_method(c,"init");if(init)call(bind_method(c,init,self),args,argc,e);else if(argc)rt_err("class %s takes no arguments\n",c->name);return self;}int saved_line=g_line;if(fn.t!=V_FUNC){rt_err("value is not callable\n");exit(3);}Func*f=fn.f;if(argc!=f->count){rt_err("function expected %d arguments, got %d\n",f->count,argc);exit(3);}Env*local=env_new(f->closure?f->closure:e);for(int i=0;i<argc;i++)define(local,f->names[i],args[i]);bool old=returning;int old_floor=loop_floor;loop_floor=loop_depth;returning=false;Value r=nul();for(Node*x=f->body->next;x&&!returning&&!loop_signal;x=x->next)r=eval(x,local);loop_floor=old_floor;g_line=saved_line;if(returning){r=retvalue;returning=old;}return r;}
#define ARITY(z) if(c!=(z)){rt_err("%s.%s expects %d arguments\n",m,fn,z);exit(3);}
 #ifdef MINT_WITH_RAYLIB
static int mint_key_code(const char *name) {
    if (!strcmp(name,"escape")) return KEY_ESCAPE;
    if (!strcmp(name,"space")) return KEY_SPACE;
    if (!strcmp(name,"enter")) return KEY_ENTER;
    if (!strcmp(name,"tab")) return KEY_TAB;
    if (!strcmp(name,"left")) return KEY_LEFT;
    if (!strcmp(name,"right")) return KEY_RIGHT;
    if (!strcmp(name,"up")) return KEY_UP;
    if (!strcmp(name,"down")) return KEY_DOWN;
    if (!strcmp(name,"backspace")) return KEY_BACKSPACE;
    if (strlen(name)==1 && isalpha((unsigned char)name[0]))
        return toupper((unsigned char)name[0]);
    if (strlen(name)>=2 && name[0]=='f') {
        int n=atoi(name+1);
        if(n>=1 && n<=12) return KEY_F1+n-1;
    }
    return -1;
}
static int mint_mouse_code(const char *name) {
    if (!strcmp(name,"left")) return MOUSE_BUTTON_LEFT;
    if (!strcmp(name,"right")) return MOUSE_BUTTON_RIGHT;
    if (!strcmp(name,"middle")) return MOUSE_BUTTON_MIDDLE;
    return -1;
}
static Color mint_color(Value *a) {
    return (Color){(unsigned char)a[0].n,(unsigned char)a[1].n,
                   (unsigned char)a[2].n,255};
}
#endif
static bool values_equal(Value a,Value b){if(a.t!=b.t)return false;switch(a.t){case V_NUM:return a.n==b.n;case V_BOOL:return a.b==b.b;case V_STR:return !strcmp(a.s,b.s);case V_NULL:return true;case V_DICT:return a.dict==b.dict;default:return a.list==b.list;}}
static int compare_values(const void*x,const void*y){const Value*a=x,*b=y;if(a->t==V_STR)return strcmp(a->s,b->s);return (a->n>b->n)-(a->n<b->n);}
static char* replace_all(const char*s,const char*old,const char*nw){size_t ol=strlen(old),nl=strlen(nw);if(ol==0)return strdup(s);size_t count=0;for(const char*p=s;(p=strstr(p,old))!=NULL;p+=ol)count++;char*out=malloc(strlen(s)-count*ol+count*nl+1);char*w=out;const char*p=s;const char*q;while((q=strstr(p,old))!=NULL){size_t k=(size_t)(q-p);memcpy(w,p,k);w+=k;memcpy(w,nw,nl);w+=nl;p=q+ol;}strcpy(w,p);return out;}
typedef struct {char*s;size_t n,cap;} Buf;
static void buf_add(Buf*b,const char*x,size_t k){if(b->n+k+1>b->cap){b->cap=(b->n+k+1)*2;b->s=realloc(b->s,b->cap);}memcpy(b->s+b->n,x,k);b->n+=k;b->s[b->n]=0;}
static void buf_str(Buf*b,const char*x){buf_add(b,x,strlen(x));}
static void json_string(Buf*b,const char*s){buf_add(b,"\"",1);for(const char*p=s;*p;p++){switch(*p){case '"':buf_str(b,"\\\"");break;case '\\':buf_str(b,"\\\\");break;case '\n':buf_str(b,"\n");break;case '\t':buf_str(b,"\\t");break;case '\r':buf_str(b,"\\r");break;default:buf_add(b,p,1);}}buf_add(b,"\"",1);}
static void json_encode(Buf*b,Value v){switch(v.t){case V_NULL:buf_str(b,"null");break;case V_BOOL:buf_str(b,v.b?"true":"false");break;case V_NUM:{char tmp[64];if(v.n==floor(v.n)&&fabs(v.n)<1e15)snprintf(tmp,sizeof tmp,"%.0f",v.n);else{snprintf(tmp,sizeof tmp,"%.15g",v.n);if(strtod(tmp,NULL)!=v.n)snprintf(tmp,sizeof tmp,"%.17g",v.n);}buf_str(b,tmp);break;}case V_STR:json_string(b,v.s);break;case V_LIST:buf_add(b,"[",1);for(size_t i=0;i<v.list->len;i++){if(i)buf_add(b,",",1);json_encode(b,v.list->items[i]);}buf_add(b,"]",1);break;case V_DICT:buf_add(b,"{",1);for(size_t i=0;i<v.dict->len;i++){if(i)buf_add(b,",",1);json_string(b,v.dict->keys[i]);buf_add(b,":",1);json_encode(b,v.dict->values[i]);}buf_add(b,"}",1);break;default:rt_err("json.encode cannot encode this value\n");exit(3);}}
static void jfail(void){rt_err("json.decode: invalid JSON text\n");exit(3);}
static void jskip(const char**p){while(**p==' '||**p=='\n'||**p=='\t'||**p=='\r')(*p)++;}
static Value jparse(const char**p){jskip(p);char c=**p;
 if(c=='{'){(*p)++;Value v=dict_value();jskip(p);if(**p=='}'){(*p)++;return v;}for(;;){jskip(p);if(**p!='"')jfail();Value k=jparse(p);jskip(p);if(**p!=':')jfail();(*p)++;Value val=jparse(p);dict_set(v.dict,k.s,val);jskip(p);if(**p==','){(*p)++;continue;}if(**p=='}'){(*p)++;return v;}jfail();}}
 if(c=='['){(*p)++;Value v=list_value();jskip(p);if(**p==']'){(*p)++;return v;}for(;;){list_push(v.list,jparse(p));jskip(p);if(**p==','){(*p)++;continue;}if(**p==']'){(*p)++;return v;}jfail();}}
 if(c=='"'){(*p)++;Buf b={0};while(**p&&**p!='"'){if(**p=='\\'){(*p)++;switch(**p){case 'n':buf_add(&b,"\n",1);break;case 't':buf_add(&b,"\t",1);break;case 'r':buf_add(&b,"\r",1);break;case '"':case '\\':case '/':buf_add(&b,*p,1);break;default:jfail();}(*p)++;}else{buf_add(&b,*p,1);(*p)++;}}if(**p!='"')jfail();(*p)++;return str(b.s?b.s:"");}
 if(strncmp(*p,"true",4)==0){*p+=4;return boolean(true);}
 if(strncmp(*p,"false",5)==0){*p+=5;return boolean(false);}
 if(strncmp(*p,"null",4)==0){*p+=4;return nul();}
 if(c=='-'||isdigit((unsigned char)c)){char*end;double d=strtod(*p,&end);if(end==*p)jfail();*p=end;return num(d);}
 jfail();return nul();}
static Value json_module(const char*fn,Value*a,int c){
 if(!strcmp(fn,"encode")){if(c!=1){rt_err("json.encode expects one value\n");exit(3);}Buf b={0};json_encode(&b,a[0]);return str(b.s?b.s:"");}
 if(!strcmp(fn,"decode")){if(c!=1||a[0].t!=V_STR){rt_err("json.decode expects one string\n");exit(3);}const char*p=a[0].s;Value v=jparse(&p);jskip(&p);if(*p)jfail();return v;}
 rt_err("json.%s is not a function\n",fn);exit(3);}
static Value time_module(const char*fn,Value*a,int c){
 if(!strcmp(fn,"now")){if(c){rt_err("time.now expects no arguments\n");exit(3);}struct timespec ts;clock_gettime(CLOCK_REALTIME,&ts);return num((double)ts.tv_sec+ts.tv_nsec/1e9);}
 if(!strcmp(fn,"sleep")){if(c!=1||a[0].t!=V_NUM||a[0].n<0){rt_err("time.sleep expects a non-negative number of seconds\n");exit(3);}struct timespec ts;ts.tv_sec=(time_t)a[0].n;ts.tv_nsec=(long)((a[0].n-(double)ts.tv_sec)*1e9);nanosleep(&ts,NULL);return nul();}
 rt_err("time.%s is not a function\n",fn);exit(3);}
static Value path_module(const char*fn,Value*a,int c){
 if(!strcmp(fn,"join")){if(c!=2){rt_err("path.join expects two arguments\n");exit(3);}char*x=stringify(a[0]),*y=stringify(a[1]);if(y[0]=='/'||x[0]==0)return str(y);size_t xl=strlen(x);if(xl&&x[xl-1]=='/')return str(strcat(strcpy(malloc(xl+strlen(y)+1),x),y));Buf b={0};buf_str(&b,x);buf_str(&b,"/");buf_str(&b,y);return str(b.s);}
 if(c!=1){rt_err("path.%s expects one argument\n",fn);exit(3);}
 char*s=stringify(a[0]);
 if(!strcmp(fn,"basename")){char*slash=strrchr(s,'/');return str(slash?slash+1:s);}
 if(!strcmp(fn,"dirname")){char*slash=strrchr(s,'/');if(!slash)return str(".");if(slash==s)return str("/");return str(slice(s,(size_t)(slash-s)));}
 if(!strcmp(fn,"extension")){char*base=strrchr(s,'/');base=base?base+1:s;char*dot=strrchr(base,'.');if(!dot||dot==base)return str("");return str(dot);}
 rt_err("path.%s is not a function\n",fn);exit(3);}
static Value module_call(const char*m,const char*fn,Value*a,int c){
 if(!strcmp(m,"time"))return time_module(fn,a,c);
 if(!strcmp(m,"path"))return path_module(fn,a,c);
 if(!strcmp(m,"json"))return json_module(fn,a,c);
 if(!strcmp(m,"math")&&!strcmp(fn,"random")){static bool seeded=false;if(!seeded){srand((unsigned)time(NULL));seeded=true;}double unit=(double)rand()/((double)RAND_MAX+1.0);if(c==0)return num(unit);if(c==2){double lo=ceil(a[0].n),hi=floor(a[1].n);if(hi<lo){rt_err("math.random needs low <= high\n");exit(3);}return num(lo+floor(unit*(hi-lo+1)));}rt_err("math.random expects no arguments or (low, high)\n");exit(3);}
if(!strcmp(m,"math")){if(!strcmp(fn,"clamp")){ARITY(3)if(a[0].t!=V_NUM||a[1].t!=V_NUM||a[2].t!=V_NUM||a[1].n>a[2].n){rt_err("math.clamp needs numbers with low <= high\n");exit(3);}double x=a[0].n;return num(x<a[1].n?a[1].n:(x>a[2].n?a[2].n:x));}if(!strcmp(fn,"log")||!strcmp(fn,"exp")){ARITY(1)if(a[0].t!=V_NUM){rt_err("math.%s needs a number\n",fn);exit(3);}return num(!strcmp(fn,"log")?log(a[0].n):exp(a[0].n));}if(!strcmp(fn,"pi")){if(c!=0){rt_err("math.pi expects 0 arguments\n");exit(3);}return num(acos(-1));} if(!strcmp(fn,"pow")||!strcmp(fn,"min")||!strcmp(fn,"max")){if(c!=2){rt_err("math.%s expects 2 arguments\n",fn);exit(3);}if(!strcmp(fn,"pow"))return num(pow(a[0].n,a[1].n));return num(!strcmp(fn,"min")?fmin(a[0].n,a[1].n):fmax(a[0].n,a[1].n));} ARITY(1) if(!strcmp(fn,"sqrt"))return num(sqrt(a[0].n));if(!strcmp(fn,"abs"))return num(fabs(a[0].n));if(!strcmp(fn,"floor"))return num(floor(a[0].n));if(!strcmp(fn,"ceil"))return num(ceil(a[0].n));if(!strcmp(fn,"sin"))return num(sin(a[0].n));if(!strcmp(fn,"cos"))return num(cos(a[0].n));if(!strcmp(fn,"tan"))return num(tan(a[0].n));if(!strcmp(fn,"round"))return num(round(a[0].n));}
 if(!strcmp(m,"string")){if(c<1){rt_err("string.%s expects arguments\n",fn);exit(3);}char*s=stringify(a[0]);if(!strcmp(fn,"upper")||!strcmp(fn,"lower")){ARITY(1)for(char*p=s;*p;p++)*p=(char)(!strcmp(fn,"upper")?toupper((unsigned char)*p):tolower((unsigned char)*p));return str(s);}if(!strcmp(fn,"length")){ARITY(1)return num(strlen(s));}if(!strcmp(fn,"contains")){if(c!=2){rt_err("string.contains expects 2 arguments\n");exit(3);}return boolean(strstr(a[0].s,a[1].s)!=NULL);}if(!strcmp(fn,"trim")){ARITY(1)char*p=s;while(isspace((unsigned char)*p))p++;char*end=p+strlen(p);while(end>p&&isspace((unsigned char)end[-1]))end--;return str(slice(p,(size_t)(end-p)));}if(!strcmp(fn,"starts_with")||!strcmp(fn,"ends_with")){ARITY(2)char*t=stringify(a[1]);size_t sl=strlen(s),tl=strlen(t);bool hit=!strcmp(fn,"starts_with")?(tl<=sl&&strncmp(s,t,tl)==0):(tl<=sl&&strcmp(s+sl-tl,t)==0);return boolean(hit);}if(!strcmp(fn,"split")){ARITY(2)char*t=stringify(a[1]);Value v=list_value();size_t tl=strlen(t);if(tl==0){for(size_t i=0;s[i];i++){char one[2]={s[i],0};list_push(v.list,str(one));}return v;}const char*p=s;const char*q;while((q=strstr(p,t))!=NULL){list_push(v.list,str(slice(p,(size_t)(q-p))));p=q+tl;}list_push(v.list,str(p));return v;}if(!strcmp(fn,"repeat")){ARITY(2)if(a[1].t!=V_NUM||a[1].n<0){rt_err("string.repeat expects a non-negative count\n");exit(3);}Buf b={0};for(long k=0;k<(long)a[1].n;k++)buf_str(&b,s);return str(b.s?b.s:"");}if(!strcmp(fn,"index_of")){ARITY(2)char*t=stringify(a[1]);char*hit=strstr(s,t);return num(hit?(double)(hit-s):-1);}if(!strcmp(fn,"replace")){ARITY(3)char*t=stringify(a[1]);char*u=stringify(a[2]);return str(replace_all(s,t,u));}}
 if(!strcmp(m,"fs")){if(!strcmp(fn,"lines")){ARITY(1)FILE*f=fopen(a[0].s,"r");if(!f){rt_err("fs.lines cannot open %s\n",a[0].s);exit(3);}Value v=list_value();char*line=NULL;size_t cap=0;ssize_t len;while((len=getline(&line,&cap,f))!=-1){if(len&&line[len-1]=='\n')line[--len]=0;if(len&&line[len-1]=='\r')line[len-1]=0;list_push(v.list,str(line));}free(line);fclose(f);return v;}if(!strcmp(fn,"exists")){ARITY(1)struct stat st;return boolean(stat(a[0].s,&st)==0);}if(!strcmp(fn,"read")){ARITY(1)FILE*f=fopen(a[0].s,"rb");if(!f)return str("");fseek(f,0,SEEK_END);long z=ftell(f);rewind(f);char*s=calloc((size_t)z+1,1);size_t got=fread(s,1,(size_t)z,f);s[got]=0;fclose(f);return str(s);}if(!strcmp(fn,"write")||!strcmp(fn,"append")){ARITY(2)FILE*f=fopen(a[0].s,!strcmp(fn,"write")?"wb":"ab");if(!f)return boolean(false);char*s=stringify(a[1]);fputs(s,f);fclose(f);return boolean(true);}if(!strcmp(fn,"remove")){ARITY(1)return boolean(remove(a[0].s)==0);}if(!strcmp(fn,"mkdir")){ARITY(1)return boolean(mkdir(a[0].s,0775)==0);}if(!strcmp(fn,"is_dir")){ARITY(1)struct stat st;return boolean(stat(a[0].s,&st)==0&&S_ISDIR(st.st_mode));}if(!strcmp(fn,"size")){ARITY(1)struct stat st;if(stat(a[0].s,&st)!=0)return num(-1);return num((double)st.st_size);}if(!strcmp(fn,"copy")){ARITY(2)FILE*in=fopen(a[0].s,"rb");if(!in)return boolean(false);FILE*out=fopen(a[1].s,"wb");if(!out){fclose(in);return boolean(false);}char buf[16384];size_t n;bool ok=true;while((n=fread(buf,1,sizeof buf,in))>0){if(fwrite(buf,1,n,out)!=n){ok=false;break;}}if(ferror(in))ok=false;if(fclose(in)!=0)ok=false;if(fclose(out)!=0)ok=false;if(!ok)remove(a[1].s);return boolean(ok);} }
  if(!strcmp(m,"gui")){
#ifdef MINT_WITH_RAYLIB
    if(!strcmp(fn,"open")){ARITY(3) InitWindow((int)a[0].n,(int)a[1].n,a[2].s);return nul();}
    if(!strcmp(fn,"close")){ARITY(0) CloseWindow();return nul();}
    if(!strcmp(fn,"should_close")){ARITY(0) return boolean(WindowShouldClose());}
    if(!strcmp(fn,"begin")){ARITY(0) BeginDrawing();return nul();}
    if(!strcmp(fn,"end")){ARITY(0) EndDrawing();return nul();}
    if(!strcmp(fn,"clear")){ARITY(3) ClearBackground(mint_color(a));return nul();}
    if(!strcmp(fn,"text")){ARITY(7) DrawText(a[0].s,(int)a[1].n,(int)a[2].n,(int)a[3].n,mint_color(a+4));return nul();}
    if(!strcmp(fn,"rect")){ARITY(7) DrawRectangle((int)a[0].n,(int)a[1].n,(int)a[2].n,(int)a[3].n,mint_color(a+4));return nul();}
    if(!strcmp(fn,"circle")){ARITY(6) DrawCircle((int)a[0].n,(int)a[1].n,(float)a[2].n,mint_color(a+3));return nul();}
    if(!strcmp(fn,"key_down")||!strcmp(fn,"key_pressed")){ARITY(1) int key=mint_key_code(a[0].s);if(key<0)return boolean(false);return boolean(!strcmp(fn,"key_down")?IsKeyDown(key):IsKeyPressed(key));}
    if(!strcmp(fn,"mouse_x")){ARITY(0) return num(GetMouseX());}
    if(!strcmp(fn,"mouse_y")){ARITY(0) return num(GetMouseY());}
    if(!strcmp(fn,"mouse_pressed")){ARITY(1) int button=mint_mouse_code(a[0].s);return boolean(button>=0&&IsMouseButtonPressed(button));}
    if(!strcmp(fn,"fps")){ARITY(1) SetTargetFPS((int)a[0].n);return nul();}
    if(!strcmp(fn,"line")){ARITY(7) DrawLine((int)a[0].n,(int)a[1].n,(int)a[2].n,(int)a[3].n,mint_color(a+4));return nul();}
    if(!strcmp(fn,"rect_outline")){ARITY(7) DrawRectangleLines((int)a[0].n,(int)a[1].n,(int)a[2].n,(int)a[3].n,mint_color(a+4));return nul();}
    if(!strcmp(fn,"measure_text")){ARITY(2) return num(MeasureText(a[0].s,(int)a[1].n));}
    if(!strcmp(fn,"window_width")){ARITY(0) return num(GetScreenWidth());}
    if(!strcmp(fn,"window_height")){ARITY(0) return num(GetScreenHeight());}
    if(!strcmp(fn,"mouse_wheel")){ARITY(0) return num(GetMouseWheelMove());}
    if(!strcmp(fn,"mouse_down")){ARITY(1) int button=mint_mouse_code(a[0].s);return boolean(button>=0&&IsMouseButtonDown(button));}
    if(!strcmp(fn,"set_title")){ARITY(1) SetWindowTitle(a[0].s);return nul();}
    if(!strcmp(fn,"toggle_fullscreen")){ARITY(0) ToggleFullscreen();return nul();}
    if(!strcmp(fn,"button")){
      ARITY(11)
      int x=(int)a[1].n,y=(int)a[2].n,w=(int)a[3].n,h=(int)a[4].n;
      int mx=GetMouseX(),my=GetMouseY();
      bool hover=mx>=x&&mx<=x+w&&my>=y&&my<=y+h;
      Color bg=mint_color(a+5),fg=mint_color(a+8);
      if(hover){bg.r=(unsigned char)(bg.r>230?255:bg.r+25);bg.g=(unsigned char)(bg.g>230?255:bg.g+25);bg.b=(unsigned char)(bg.b>230?255:bg.b+25);}
      DrawRectangle(x,y,w,h,bg);
      int tw=MeasureText(a[0].s,20);
      DrawText(a[0].s,x+(w-tw)/2,y+(h-20)/2,20,fg);
      return boolean(hover&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT));
    }
#else
    rt_err("GUI module unavailable; install raylib development files and rebuild with make raylib\n");exit(3);
#endif
  }
 if(!strcmp(m,"sqlite")){
#ifdef MINT_WITH_SQLITE
   if(!strcmp(fn,"open")){ARITY(1) sqlite3*db=NULL;int rc=sqlite3_open(a[0].s,&db);if(rc!=SQLITE_OK){fprintf(stderr,"MINT sqlite error: %s\n",db?sqlite3_errmsg(db):"open failed");if(db)sqlite3_close(db);exit(3);}Value v={.t=V_DB,.ptr=db};return v;}
#else
   rt_err("SQLite module unavailable; install libsqlite3-dev and rebuild MINT\n");exit(3);
#endif
 }
 if(!strcmp(m,"system")){if(!strcmp(fn,"cwd")){ARITY(0)char b[4096];return str(getcwd(b,sizeof b)?b:"");}if(!strcmp(fn,"env")){ARITY(1)char*s=getenv(a[0].s);return str(s?s:"");}}
 rt_err("unknown function %s.%s\n",m,fn);exit(3);}
#undef ARITY
static bool member_of(Value item,Value container){
 if(container.t==V_LIST){for(size_t i=0;i<container.list->len;i++){Value x=container.list->items[i];if(x.t!=item.t)continue;if(item.t==V_NUM&&x.n==item.n)return true;if(item.t==V_STR&&!strcmp(x.s,item.s))return true;if(item.t==V_BOOL&&x.b==item.b)return true;if(item.t==V_NULL)return true;}return false;}
 if(container.t==V_DICT){char*k=stringify(item);return dict_find(container.dict,k)>=0;}
 if(container.t==V_STR){if(item.t!=V_STR)rt_err("'in' needs text on the left when searching text\n");return strstr(container.s,item.s)!=NULL;}
 rt_err("'in' needs a list, dictionary or text on the right\n");return false;}
static Value eval(Node*n,Env*e){if(!n)return nul();if(n->line)g_line=n->line;switch(n->t){case N_NUM:case N_STR:case N_BOOL:return n->val;case N_NULL:return nul();case N_LIST:{Value v=list_value();for(int i=0;i<n->argc;i++)list_push(v.list,eval(n->args[i],e));return v;}case N_DICT:{Value v=dict_value();for(int i=0;i<n->argc;i+=2){Value k=eval(n->args[i],e);char*key=stringify(k);dict_set(v.dict,key,eval(n->args[i+1],e));}return v;}case N_INDEX:{Value base=eval(n->a,e),idx=eval(n->b,e);if(base.t==V_LIST)return base.list->items[list_index(idx,base.list->len)];if(base.t==V_DICT){char*k=stringify(idx);long i=dict_find(base.dict,k);if(i<0){rt_err("missing dictionary key '%s'\n",k);exit(3);}return base.dict->values[i];}if(base.t==V_INST)return instance_member(base,stringify(idx));if(base.t==V_STR&&idx.t==V_NUM){long i=list_index(idx,strlen(base.s));char c[2]={base.s[i],0};return str(c);}rt_err("value is not indexable\n");exit(3);}case N_INDEX_SET:{Value base=eval(n->a,e),idx=eval(n->b,e),v=eval(n->c,e);if(base.t==V_LIST){base.list->items[list_index(idx,base.list->len)]=v;return v;}if(base.t==V_DICT){char*k=stringify(idx);dict_set(base.dict,k,v);return v;}if(base.t==V_INST){dict_set(((Instance*)base.ptr)->fields.dict,stringify(idx),v);return v;}rt_err("indexed assignment needs a list or dict\n");exit(3);}case N_VAR:return get(e,n->name);case N_LET:{Value v=eval(n->a,e);define(e,n->name,v);return v;}case N_SET:{Value v=eval(n->a,e);set(e,n->name,v);return v;}case N_PRINT:{char*s=stringify(eval(n->a,e));puts(s);return nul();}case N_IF:if(truth(eval(n->a,e))){Env*l=env_new(e);for(Node*x=n->b->next;x&&!returning&&!loop_signal;x=x->next)eval(x,l);}else if(n->c){Env*l=env_new(e);for(Node*x=n->c->next;x&&!returning&&!loop_signal;x=x->next)eval(x,l);}return nul();case N_WHILE:{loop_depth++;while(truth(eval(n->a,e))){Env*l=env_new(e);for(Node*x=n->b->next;x&&!returning&&!loop_signal;x=x->next)eval(x,l);if(returning)break;if(loop_signal==1){loop_signal=0;break;}if(loop_signal==2)loop_signal=0;}loop_depth--;return nul();}case N_FOR:{Value seq=eval(n->a,e);size_t count=0;if(seq.t==V_LIST)count=seq.list->len;else if(seq.t==V_STR)count=strlen(seq.s);else if(seq.t==V_DICT)count=seq.dict->len;else{rt_err("for-in needs a list, string, or dictionary\n");exit(3);}loop_depth++;for(size_t i=0;i<count&&!returning;i++){Env*l=env_new(e);Value item=seq.t==V_LIST?seq.list->items[i]:seq.t==V_STR?str((char[]){seq.s[i],0}):str(seq.dict->keys[i]);define(l,n->name,item);for(Node*x=n->b->next;x&&!returning&&!loop_signal;x=x->next)eval(x,l);if(loop_signal==1){loop_signal=0;break;}if(loop_signal==2)loop_signal=0;}loop_depth--;return nul();}case N_BREAK:if(loop_depth<=loop_floor){rt_err("break used outside a loop\n");exit(3);}loop_signal=1;return nul();case N_CONTINUE:if(loop_depth<=loop_floor){rt_err("continue used outside a loop\n");exit(3);}loop_signal=2;return nul();case N_BLOCK:{Env*l=env_new(e);for(Node*x=n->next;x&&!returning&&!loop_signal;x=x->next)eval(x,l);return nul();}case N_FSTR:{Buf b={0};for(int i=0;i<n->argc;i++){Value v=eval(n->args[i],e);char*x;if(n->params[i]&&v.t==V_NUM){char tmp[128];snprintf(tmp,sizeof tmp,"%.*f",atoi(n->params[i]+1),v.n);x=strdup(tmp);}else x=stringify(v);buf_str(&b,x);}return str(b.s?b.s:"");}case N_THROW:{Value v=eval(n->a,e);char*s=stringify(v);snprintf(err_text,sizeof err_text,"%s",s);err_line=g_line;raise_text();return nul();}case N_TRY:{int my=try_depth;int sd=loop_depth,sf=loop_floor,sl=loop_signal;bool sr=returning;jmp_buf jb;try_stack[try_depth++]=&jb;if(setjmp(jb)==0){Env*l=env_new(e);for(Node*x=n->a->next;x&&!returning&&!loop_signal;x=x->next)eval(x,l);try_depth=my;return nul();}try_depth=my;loop_depth=sd;loop_floor=sf;loop_signal=sl;returning=sr;Env*l=env_new(e);if(n->name)define(l,n->name,str(err_text));for(Node*x=n->b->next;x&&!returning&&!loop_signal;x=x->next)eval(x,l);return nul();}case N_CLASS:{Class*c=calloc(1,sizeof(Class));c->name=n->name;c->methods=n->args;c->mcount=n->argc;c->env=e;Value v={.t=V_CLASS,.ptr=c};define(e,n->name,v);return v;}case N_LAMBDA:{Func*f=calloc(1,sizeof(Func));f->names=n->params;f->count=n->pc;f->body=n->a;f->closure=e;Value v={.t=V_FUNC,.f=f};return v;}case N_FUN:{Func*f=calloc(1,sizeof(Func));f->names=n->params;f->count=n->pc;f->body=n->a;f->closure=e;Value v={.t=V_FUNC,.f=f};define(e,n->name,v);return v;}case N_RET:{Value rv=n->a?eval(n->a,e):nul();retvalue=rv;returning=true;return rv;}case N_IMPORT:{
 const char*name=n->name;
 bool builtin=!strcmp(name,"math")||!strcmp(name,"string")||!strcmp(name,"fs")||!strcmp(name,"system")||!strcmp(name,"sqlite")||!strcmp(name,"gui")||!strcmp(name,"time")||!strcmp(name,"path")||!strcmp(name,"json");
 Value v={.t=V_MOD,.s=strdup(name)};
 if(!builtin){
   FILE*mf=fopen(name,"rb");
   if(!mf){fprintf(stderr,"MINT import error: cannot open module '%s': %s\n",name,strerror(errno));exit(4);}
   if(fseek(mf,0,SEEK_END)!=0){fprintf(stderr,"MINT import error: cannot seek module '%s'\n",name);exit(4);}
   long msize=ftell(mf);rewind(mf);
   if(msize<0||msize>16*1024*1024){fprintf(stderr,"MINT import error: module '%s' is too large\n",name);exit(4);}
   char*msrc=calloc((size_t)msize+1,1);
   if(!msrc||fread(msrc,1,(size_t)msize,mf)!=(size_t)msize){fprintf(stderr,"MINT import error: could not read module '%s'\n",name);fclose(mf);exit(4);}
   fclose(mf);
   Lexer ml=lex(msrc);Node*mp=parse(&ml);Env*me=env_new(NULL);
   for(Node*x=mp->next;x&&!returning;x=x->next)eval(x,me);
   v.ptr=me;
 }
 define(e,n->op,v);return v;}case N_EXPR:{Value base=eval(n->a,e);if(n->name&&base.t==V_MOD&&base.ptr)return get((Env*)base.ptr,n->name);if(n->name&&base.t==V_INST)return instance_member(base,n->name);return base;}case N_UN:{Value v=eval(n->a,e);if(!strcmp(n->op,"not"))return boolean(!truth(v));if(v.t!=V_NUM){rt_err("unary operator needs number\n");exit(3);}return num(!strcmp(n->op,"-")?-v.n:v.n);}case N_BIN:{Value a=eval(n->a,e);if(!strcmp(n->op,"and"))return truth(a)?eval(n->b,e):a;if(!strcmp(n->op,"or"))return truth(a)?a:eval(n->b,e);Value b=eval(n->b,e);if(!strcmp(n->op,"in"))return boolean(member_of(a,b));if(!strcmp(n->op,"+")){if(a.t==V_STR||b.t==V_STR){char*x=stringify(a),*y=stringify(b),*z=malloc(strlen(x)+strlen(y)+1);strcpy(z,x);strcat(z,y);return str(z);}return num(a.n+b.n);}if(!strcmp(n->op,"-"))return num(a.n-b.n);if(!strcmp(n->op,"*"))return num(a.n*b.n);if(!strcmp(n->op,"/")){if(b.n==0){rt_err("division by zero\n");exit(3);}return num(a.n/b.n);}if(!strcmp(n->op,"%")){if(b.n==0){rt_err("modulo by zero\n");exit(3);}double r=fmod(a.n,b.n);if(r!=0&&((r<0)!=(b.n<0)))r+=b.n;return num(r);}if(!strcmp(n->op,"=="))return boolean(a.t==b.t&&(a.t==V_NUM?a.n==b.n:a.t==V_BOOL?a.b==b.b:a.t==V_STR?!strcmp(a.s,b.s):true));if(!strcmp(n->op,"!="))return boolean(!(a.t==b.t&&(a.t==V_NUM?a.n==b.n:a.t==V_BOOL?a.b==b.b:a.t==V_STR?!strcmp(a.s,b.s):true)));if(!strcmp(n->op,"<"))return boolean(a.t==V_STR?strcmp(a.s,b.s)<0:a.n<b.n);if(!strcmp(n->op,">"))return boolean(a.t==V_STR?strcmp(a.s,b.s)>0:a.n>b.n);if(!strcmp(n->op,"<="))return boolean(a.n<=b.n);return boolean(a.n>=b.n);}case N_CALL:{Value*args=calloc((size_t)n->argc,sizeof(Value));for(int i=0;i<n->argc;i++)args[i]=eval(n->args[i],e);if(n->a->t==N_EXPR&&n->a->a->t==N_VAR){Value mod=get(e,n->a->a->name);if(mod.t==V_MOD){if(mod.ptr){Env*module_env=(Env*)mod.ptr;Value target=get(module_env,n->a->name);return call(target,args,n->argc,module_env);}return module_call(mod.s,n->a->name,args,n->argc);}
#ifdef MINT_WITH_SQLITE
 if(mod.t==V_DB){sqlite3*db=(sqlite3*)mod.ptr;if(!strcmp(n->a->name,"exec")){if(n->argc!=1){rt_err("db.exec expects one argument\n");exit(3);}char*err=NULL;int rc=sqlite3_exec(db,args[0].s,NULL,NULL,&err);if(rc!=SQLITE_OK){fprintf(stderr,"MINT sqlite error: %s\n",err?err:sqlite3_errmsg(db));sqlite3_free(err);exit(3);}return boolean(true);}if(!strcmp(n->a->name,"scalar")){if(n->argc!=1){rt_err("db.scalar expects one argument\n");exit(3);}sqlite3_stmt*st=NULL;if(sqlite3_prepare_v2(db,args[0].s,-1,&st,NULL)!=SQLITE_OK){fprintf(stderr,"MINT sqlite error: %s\n",sqlite3_errmsg(db));exit(3);}Value out=nul();if(sqlite3_step(st)==SQLITE_ROW){const unsigned char*v=sqlite3_column_text(st,0);if(v)out=str((const char*)v);}sqlite3_finalize(st);return out;}if(!strcmp(n->a->name,"changes")){if(n->argc){rt_err("db.changes expects no arguments\n");exit(3);}return num(sqlite3_changes(db));}if(!strcmp(n->a->name,"last_id")){if(n->argc){rt_err("db.last_id expects no arguments\n");exit(3);}return num((double)sqlite3_last_insert_rowid(db));}if(!strcmp(n->a->name,"close")){if(n->argc){rt_err("db.close expects no arguments\n");exit(3);}int rc=sqlite3_close(db);if(rc!=SQLITE_OK){fprintf(stderr,"MINT sqlite error: %s\n",sqlite3_errstr(rc));exit(3);}return boolean(true);}}
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
   if(!strcmp(method,"has")){if(n->argc!=1){rt_err("dict.has expects one key\n");exit(3);}char*k=stringify(args[0]);return boolean(dict_find(receiver.dict,k)>=0);}
   if(!strcmp(method,"length")){if(n->argc){rt_err("dict.length expects no arguments\n");exit(3);}return num(receiver.dict->len);}
 }
 if(receiver.t==V_STR){Value*full=calloc((size_t)n->argc+1,sizeof(Value));full[0]=receiver;for(int i=0;i<n->argc;i++)full[i+1]=args[i];return module_call("string",method,full,n->argc+1);}
 if(n->a->a->t!=N_VAR&&receiver.t!=V_LIST&&receiver.t!=V_DICT){rt_err("value has no method '%s'\n",method);exit(3);}
}
if(n->a->t==N_VAR&&!strcmp(n->a->name,"range")){if(n->argc<1||n->argc>3){rt_err("range expects 1 to 3 arguments\n");exit(3);}double start=0,stop=0,step=1;if(n->argc==1)stop=args[0].n;else{start=args[0].n;stop=args[1].n;if(n->argc==3)step=args[2].n;}if(args[0].t!=V_NUM||(n->argc>1&&args[1].t!=V_NUM)||(n->argc>2&&args[2].t!=V_NUM)||floor(start)!=start||floor(stop)!=stop||floor(step)!=step||step==0){rt_err("range arguments must be integers and step cannot be zero\n");exit(3);}Value out=list_value();size_t guard=0;for(double v=start;(step>0?v<stop:v>stop);v+=step){if(++guard>1000000){rt_err("range exceeds one million items\n");exit(3);}list_push(out.list,num(v));}return out;}if(n->a->t==N_VAR&&!strcmp(n->a->name,"input")){if(n->argc>1){rt_err("input accepts zero or one argument\n");exit(3);}if(n->argc){char*p=stringify(args[0]);fputs(p,stdout);fflush(stdout);}char line[4096];if(!fgets(line,sizeof line,stdin))return str("");line[strcspn(line,"\r\n")]=0;return str(line);}if(n->a->t==N_VAR&&!strcmp(n->a->name,"type")){if(n->argc!=1){rt_err("type expects one argument\n");exit(3);}const char*t=args[0].t==V_NUM?"number":args[0].t==V_BOOL?"bool":args[0].t==V_STR?"string":args[0].t==V_FUNC?"function":args[0].t==V_MOD?"module":args[0].t==V_DB?"database":args[0].t==V_INST?"instance":args[0].t==V_CLASS?"class":args[0].t==V_LIST?"list":args[0].t==V_DICT?"dict":"null";return str(t);}if(n->a->t==N_VAR&&!strcmp(n->a->name,"int")){if(n->argc!=1){rt_err("int expects one argument\n");exit(3);}return num(trunc(args[0].t==V_STR?strtod(args[0].s,NULL):args[0].n));}if(n->a->t==N_VAR&&!strcmp(n->a->name,"float")){if(n->argc!=1){rt_err("float expects one argument\n");exit(3);}return num(args[0].t==V_STR?strtod(args[0].s,NULL):args[0].n);}if(n->a->t==N_VAR&&!strcmp(n->a->name,"len")){if(n->argc!=1){rt_err("len expects one argument\n");exit(3);}if(args[0].t==V_LIST)return num(args[0].list->len);if(args[0].t==V_DICT)return num(args[0].dict->len);char*s=stringify(args[0]);return num(strlen(s));}if(n->a->t==N_VAR&&!strcmp(n->a->name,"str")){if(n->argc!=1){rt_err("str expects one argument\n");exit(3);}return str(stringify(args[0]));}return call(eval(n->a,e),args,n->argc,e);}default:return nul();} }
int main(int argc,char**argv){if(argc<2||!strcmp(argv[1],"--help")||!strcmp(argv[1],"-h")){puts("MINT 1.0-native\nUsage: mint [--check] <program.mint>\n       mint --version\n       mint --help");return 0;}bool check=!strcmp(argv[1],"--check");if(!strcmp(argv[1],"--version")){puts("MINT 1.0-native");return 0;}const char*filename=check?(argc>2?argv[2]:NULL):argv[1];if(!filename){fprintf(stderr,"Usage: mint [--check] <program.mint>\n");return 1;}FILE*f=fopen(filename,"rb");if(!f){fprintf(stderr,"mint: cannot open %s: %s\n",filename,strerror(errno));return 1;}fseek(f,0,SEEK_END);long z=ftell(f);rewind(f);char*src=calloc((size_t)z+1,1);size_t got_src=fread(src,1,(size_t)z,f);src[got_src]=0;fclose(f);Lexer l=lex(src);Node*program=parse(&l);if(check){printf("OK: %s parsed by native MINT\n",filename);return 0;}Env*g=env_new(NULL);for(Node*n=program->next;n&&!returning;n=n->next)eval(n,g);return 0;}
