/* Kiln native runtime: a compact C implementation of the language core.
 * Build: cc -std=c11 -O2 runtime/kiln.c -lm -o kiln
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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef KILN_WITH_SQLITE
#include <sqlite3.h>
#endif
#ifdef KILN_WITH_RAYLIB
#include <raylib.h>
#endif

typedef enum { T_EOF,T_NUM,T_STR,T_ID,T_LET,T_FUN,T_RETURN,T_PRINT,T_IF,T_ELSE,T_WHILE,T_TRUE,T_FALSE,T_NULL,T_AND,T_OR,T_NOT,T_IMPORT,T_AS,T_FOR,T_IN,T_BREAK,T_CONTINUE,T_LP,T_RP,T_LB,T_RB,T_LS,T_RS,T_COMMA,T_COLON,T_DOT,T_SEMI,T_PLUS,T_MINUS,T_STAR,T_SLASH,T_PERCENT,T_EQ,T_EQEQ,T_NE,T_LT,T_GT,T_LE,T_GE,T_ARROW,T_CLASS,T_TRY,T_CATCH,T_THROW,T_FSTR,T_SLASHSLASH,T_POW,T_PLUSEQ,T_MINUSEQ,T_STAREQ,T_SLASHEQ,T_PERCENTEQ,T_FINALLY } Tok;
typedef struct { Tok t; char *s; double n; int line,col; } Token;
typedef struct { Token *v; int n,cap,pos; } Lexer;
static char *slice(const char*s,size_t n){char*r=malloc(n+1);memcpy(r,s,n);r[n]=0;return r;}
static void push(Lexer*l,Token t){if(l->n==l->cap){l->cap=l->cap?l->cap*2:128;l->v=realloc(l->v,l->cap*sizeof(Token));}l->v[l->n++]=t;}
static void lexerr(int line,int col,const char*m){fprintf(stderr,"Kiln syntax error at %d:%d: %s\n",line,col,m);exit(2);}
/* Identifiers are interned: equal names share one pointer, so scope lookups can
 * compare pointers first. Interned strings live for the whole run. */
static char**g_names=NULL;static size_t g_nnames=0,g_capnames=0;
static char* intern(char*s){
 for(size_t i=0;i<g_nnames;i++)if(!strcmp(g_names[i],s)){free(s);return g_names[i];}
 if(g_nnames==g_capnames){g_capnames=g_capnames?g_capnames*2:256;g_names=realloc(g_names,g_capnames*sizeof(char*));}
 g_names[g_nnames++]=s;return s;}
static Tok keyword(const char*s){struct{const char*s;Tok t;}k[]={{"let",T_LET},{"fun",T_FUN},{"return",T_RETURN},{"print",T_PRINT},{"if",T_IF},{"else",T_ELSE},{"while",T_WHILE},{"true",T_TRUE},{"false",T_FALSE},{"null",T_NULL},{"and",T_AND},{"or",T_OR},{"not",T_NOT},{"import",T_IMPORT},{"as",T_AS},{"for",T_FOR},{"in",T_IN},{"break",T_BREAK},{"continue",T_CONTINUE},{"class",T_CLASS},{"try",T_TRY},{"finally",T_FINALLY},{"catch",T_CATCH},{"throw",T_THROW}};for(size_t i=0;i<sizeof(k)/sizeof(*k);i++)if(!strcmp(s,k[i].s))return k[i].t;return T_ID;}
static size_t utf8_encode(unsigned long cp,char*out);
static char* unescape(const char*s,size_t n){char*out=malloc(n+1);size_t o=0;for(size_t i=0;i<n;i++){if(s[i]=='\\'&&i+1<n){i++;switch(s[i]){case 'n':out[o++]='\n';break;case 't':out[o++]='\t';break;case 'r':out[o++]='\r';break;case '0':out[o++]='\0';break;case '\\':out[o++]='\\';break;case '"':out[o++]='"';break;case 'u':if(i+1<n&&s[i+1]=='{'){size_t j=i+2;unsigned long cp=0;int digits=0;while(j<n&&isxdigit((unsigned char)s[j])){int d=isdigit((unsigned char)s[j])?s[j]-'0':tolower((unsigned char)s[j])-'a'+10;cp=cp*16+(unsigned long)d;digits++;j++;}if(digits&&j<n&&s[j]=='}'&&cp<=0x10FFFF){o+=utf8_encode(cp,out+o);i=j;break;}}out[o++]='\\';out[o++]='u';break;default:out[o++]='\\';out[o++]=s[i];}}else out[o++]=s[i];}out[o]=0;return out;}
static Lexer lex(const char*src){Lexer l={0};int line=1,col=1;for(size_t i=0;src[i];){char c=src[i];if(c==' '||c=='\t'||c=='\r'){i++;col++;continue;}if(c=='\n'){i++;line++;col=1;continue;}if(c=='#'){while(src[i]&&src[i]!='\n'){i++;col++;}continue;}Token x={.line=line,.col=col};size_t st=i;if(isdigit((unsigned char)c)){while(isdigit((unsigned char)src[i])){i++;col++;}if(src[i]=='.'&&isdigit((unsigned char)src[i+1])){i++;col++;while(isdigit((unsigned char)src[i])){i++;col++;}}if((src[i]=='e'||src[i]=='E')&&(isdigit((unsigned char)src[i+1])||((src[i+1]=='+'||src[i+1]=='-')&&isdigit((unsigned char)src[i+2])))){i++;col++;if(src[i]=='+'||src[i]=='-'){i++;col++;}while(isdigit((unsigned char)src[i])){i++;col++;}}x.t=T_NUM;x.s=slice(src+st,i-st);x.n=strtod(x.s,NULL);push(&l,x);continue;}if(c=='f'&&src[i+1]=='"'){i+=2;col+=2;st=i;while(src[i]&&src[i]!='"'){if(src[i]=='\\'&&src[i+1])i++;if(src[i]=='\n'){line++;col=0;}i++;col++;}if(!src[i])lexerr(line,col,"unterminated f-string");x.s=slice(src+st,i-st);i++;col++;x.t=T_FSTR;push(&l,x);continue;}if(c=='r'&&src[i+1]=='"'){i+=2;col+=2;st=i;while(src[i]&&src[i]!='"'){if(src[i]=='\n'){line++;col=0;}i++;col++;}if(!src[i])lexerr(line,col,"unterminated raw string");x.s=slice(src+st,i-st);i++;col++;x.t=T_STR;push(&l,x);continue;}if(c=='"'&&src[i+1]=='"'&&src[i+2]=='"'){i+=3;col+=3;st=i;while(src[i]&&!(src[i]=='"'&&src[i+1]=='"'&&src[i+2]=='"')){if(src[i]=='\\'&&src[i+1])i++;if(src[i]=='\n'){line++;col=0;}i++;col++;}if(!src[i])lexerr(line,col,"unterminated triple-quoted string");x.s=unescape(src+st,i-st);i+=3;col+=3;x.t=T_STR;push(&l,x);continue;}if(isalpha((unsigned char)c)||c=='_'){while(isalnum((unsigned char)src[i])||src[i]=='_'){i++;col++;}x.s=slice(src+st,i-st);x.t=keyword(x.s);if(x.t==T_ID)x.s=intern(x.s);push(&l,x);continue;}if(c=='"'){i++;col++;st=i;while(src[i]&&src[i]!='"'){if(src[i]=='\\'&&src[i+1])i++;if(src[i]=='\n'){line++;col=0;}i++;col++;}if(!src[i])lexerr(line,col,"unterminated string");x.s=unescape(src+st,i-st);i++;col++;x.t=T_STR;push(&l,x);continue;}i++;col++;switch(c){case '(':x.t=T_LP;break;case ')':x.t=T_RP;break;case '{':x.t=T_LB;break;case '}':x.t=T_RB;break;case '[':x.t=T_LS;break;case ']':x.t=T_RS;break;case ',':x.t=T_COMMA;break;case ':':x.t=T_COLON;break;case '.':x.t=T_DOT;break;case ';':x.t=T_SEMI;break;case '+':if(src[i]=='='){i++;col++;x.t=T_PLUSEQ;}else x.t=T_PLUS;break;case '-':if(src[i]=='='){i++;col++;x.t=T_MINUSEQ;}else x.t=T_MINUS;break;case '*':if(src[i]=='*'){i++;col++;x.t=T_POW;}else if(src[i]=='='){i++;col++;x.t=T_STAREQ;}else x.t=T_STAR;break;case '/':if(src[i]=='/'){i++;col++;x.t=T_SLASHSLASH;}else if(src[i]=='='){i++;col++;x.t=T_SLASHEQ;}else x.t=T_SLASH;break;case '%':if(src[i]=='='){i++;col++;x.t=T_PERCENTEQ;}else x.t=T_PERCENT;break;case '=':if(src[i]=='='){i++;col++;x.t=T_EQEQ;}else if(src[i]=='>'){i++;col++;x.t=T_ARROW;}else x.t=T_EQ;break;case '!':if(src[i++]=='='){col++;x.t=T_NE;}else lexerr(line,col,"expected !=");break;case '<':if(src[i]=='='){i++;col++;x.t=T_LE;}else x.t=T_LT;break;case '>':if(src[i]=='='){i++;col++;x.t=T_GE;}else x.t=T_GT;break;default:lexerr(line,col,"unexpected character");}x.s=slice(src+st,i-st);push(&l,x);}push(&l,(Token){.t=T_EOF,.line=line,.col=col});return l;}

