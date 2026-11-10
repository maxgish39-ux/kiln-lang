#include <stdint.h>
typedef enum{V_NULL,V_NUM,V_BOOL,V_STR,V_FUNC,V_MOD,V_DB,V_LIST,V_DICT,V_INST,V_CLASS} VType;
typedef struct Value Value;typedef struct Node Node;typedef struct Env Env;
typedef struct {char**names;int count;Node*body;Env*closure;Node**defaults;} Func;
typedef struct {Value *items;size_t len,cap;} List;
typedef struct {char **keys;Value *values;size_t len,cap;} Dict;
typedef struct {char*name;Env*env;} ModRef;
struct Value{VType t;uint32_t len;union{double n;bool b;char*s;Func*f;void*ptr;List*list;Dict*dict;};};
static Value nul(void){return(Value){.t=V_NULL};}static Value num(double n){return(Value){.t=V_NUM,.n=n};}static Value boolean(bool b){return(Value){.t=V_BOOL,.b=b};}/* cc_bytes/cc_chars: character count of the first cc_bytes bytes.
 * cc_ch/cc_off: the byte offset of character cc_ch. Both stay valid as text is appended. */
typedef struct {size_t used,cap,ascii,cc_bytes,cc_chars,cc_ch,cc_off;} SBuf;
/* UTF-8 helpers. Text is stored as bytes; these step over whole characters.
 * Invalid bytes count as one character each. */
static size_t utf8_char_width(const char*s,size_t i,size_t n){
 unsigned char c=(unsigned char)s[i];size_t w;
 if(c<0xC0)w=1;else if(c<0xE0)w=2;else if(c<0xF0)w=3;else if(c<0xF8)w=4;else w=1;
 if(i+w>n)w=n-i;
 return w;}
static size_t leading_ascii(const char*s,size_t n){size_t i=0;while(i<n&&(unsigned char)s[i]<0x80)i++;return i;}
static size_t utf8_count(const char*s,size_t n){size_t k=0;for(size_t i=0;i<n;k++)i+=utf8_char_width(s,i,n);return k;}
static size_t utf8_encode(unsigned long cp,char*out){
 if(cp<0x80){out[0]=(char)cp;return 1;}
 if(cp<0x800){out[0]=(char)(0xC0|(cp>>6));out[1]=(char)(0x80|(cp&0x3F));return 2;}
 if(cp<0x10000){out[0]=(char)(0xE0|(cp>>12));out[1]=(char)(0x80|((cp>>6)&0x3F));out[2]=(char)(0x80|(cp&0x3F));return 3;}
 out[0]=(char)(0xF0|(cp>>18));out[1]=(char)(0x80|((cp>>12)&0x3F));out[2]=(char)(0x80|((cp>>6)&0x3F));out[3]=(char)(0x80|(cp&0x3F));return 4;}
static Value strv_n(const char*s,size_t n){if(n>UINT32_MAX){fputs("Kiln: text is too large\n",stderr);exit(3);}size_t cap=n<16?16:n;SBuf*h=malloc(sizeof(SBuf)+cap+1);h->used=n;h->cap=cap;h->cc_bytes=0;h->cc_chars=0;h->cc_ch=0;h->cc_off=0;char*d=(char*)(h+1);if(n)memcpy(d,s,n);d[n]=0;h->ascii=leading_ascii(d,n);return(Value){.t=V_STR,.len=(uint32_t)n,.s=d};}
static Value str(const char*s){return strv_n(s?s:"",s?strlen(s):0);}
/* A NUL-terminated copy of a string value. The buffer is only NUL-terminated at
 * its committed end, so older (shorter) values are copied when a C function needs them. */
static char* cstr_copy(Value v){char*c=malloc((size_t)v.len+1);memcpy(c,v.s,v.len);c[v.len]=0;return c;}
static char* cstr(Value v){if(v.t!=V_STR)return NULL;if(v.s[v.len]==0)return v.s;return cstr_copy(v);}
static int str_cmp(Value a,Value b){size_t n=a.len<b.len?a.len:b.len;int c=memcmp(a.s,b.s,n);if(c)return c;return (a.len>b.len)-(a.len<b.len);}
static bool str_eq(Value a,Value b){return a.len==b.len&&memcmp(a.s,b.s,a.len)==0;}
static bool str_ascii(Value v){SBuf*h=(SBuf*)v.s-1;return v.len<=h->ascii;}
static size_t text_len(Value v){
 if(str_ascii(v))return v.len;
 SBuf*h=(SBuf*)v.s-1;
 if(h->cc_bytes!=v.len){h->cc_bytes=v.len;h->cc_chars=utf8_count(v.s,v.len);}
 return h->cc_chars;}
/* Byte offset of character number ch (clamped to the end). Starts from the last
 * position asked for, so walking through text one character at a time stays fast. */
static size_t text_offset(Value v,size_t ch){
 if(str_ascii(v))return ch<v.len?ch:v.len;
 SBuf*h=(SBuf*)v.s-1;
 size_t k=0,i=0;
 if(h->cc_ch<=ch){k=h->cc_ch;i=h->cc_off;}
 for(;k<ch&&i<v.len;k++)i+=utf8_char_width(v.s,i,v.len);
 if(i<=v.len){h->cc_ch=k;h->cc_off=i;}
 return i;}
static long str_find(Value hay,Value needle){if(needle.len==0)return 0;size_t n=needle.len;for(size_t i=0;i+n<=(size_t)hay.len;i++)if(memcmp(hay.s+i,needle.s,n)==0)return (long)i;return -1;}
typedef struct Class Class;typedef struct Instance Instance;
struct Class{char*name;Node**methods;int mcount;Env*env;};
struct Instance{Class*cls;Value fields;};
static char* stringify(Value v){char b[96];switch(v.t){case V_NULL:return strdup("null");case V_BOOL:return strdup(v.b?"true":"false");case V_NUM:snprintf(b,sizeof b,"%.15g",v.n);return strdup(b);case V_STR:{char*c=malloc((size_t)v.len+1);memcpy(c,cstr(v),v.len);c[v.len]=0;return c;}case V_FUNC:return strdup("<function>");case V_MOD:return strdup("<module>");case V_DB:return strdup("<database>");case V_INST:{char*out=malloc(strlen(((Instance*)v.ptr)->cls->name)+16);sprintf(out,"<%s instance>",((Instance*)v.ptr)->cls->name);return out;}case V_CLASS:{char*out=malloc(strlen(((Class*)v.ptr)->name)+16);sprintf(out,"<class %s>",((Class*)v.ptr)->name);return out;}case V_LIST:{size_t cap=32;char*out=calloc(cap,1);strcat(out,"[");for(size_t i=0;i<v.list->len;i++){char*x=stringify(v.list->items[i]);size_t need=strlen(out)+strlen(x)+4;if(need>cap){cap=need*2;out=realloc(out,cap);}if(i)strcat(out,", ");strcat(out,x);}strcat(out,"]");return out;}case V_DICT:{size_t cap=32;char*out=calloc(cap,1);strcat(out,"{");for(size_t i=0;i<v.dict->len;i++){char*x=stringify(str(v.dict->keys[i]));char*y=stringify(v.dict->values[i]);size_t need=strlen(out)+strlen(x)+strlen(y)+6;if(need>cap){cap=need*2;out=realloc(out,cap);}if(i)strcat(out,", ");strcat(out,x);strcat(out,": ");strcat(out,y);}strcat(out,"}");return out;}}return strdup("?");}
static bool truth(Value v){return v.t!=V_NULL&&(v.t!=V_BOOL||v.b)&&(v.t!=V_NUM||v.n!=0)&&(v.t!=V_STR||v.len>0)&&(v.t!=V_LIST||v.list->len>0)&&(v.t!=V_DICT||v.dict->len>0);}
typedef enum{N_NUM,N_STR,N_BOOL,N_NULL,N_VAR,N_BIN,N_UN,N_LET,N_SET,N_PRINT,N_IF,N_WHILE,N_BLOCK,N_FUN,N_CALL,N_RET,N_IMPORT,N_EXPR,N_FOR,N_BREAK,N_CONTINUE,N_LIST,N_DICT,N_INDEX,N_INDEX_SET,N_LAMBDA,N_CLASS,N_TRY,N_THROW,N_FSTR,N_SLICE,N_COND,N_UNPACK} NType;
struct Node{NType t;char*name;char*op;Value val;Node*a,*b,*c,*next;Node**args;int argc;char**params;int pc;int line;int opc;void*code;int captures;};
