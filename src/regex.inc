/* Regular expressions (POSIX extended syntax, from the C library; no extra dependency).
 *
 * Patterns use POSIX extended regular expression syntax: . * + ? | () [] ^ $.
 * \d and \w are not available; write [0-9] and [A-Za-z0-9_] instead.
 */
#include <regex.h>

static regex_t* regex_compile(const char*pattern){
 regex_t*re=malloc(sizeof *re);
 int rc=regcomp(re,pattern,REG_EXTENDED);
 if(rc){char msg[256];regerror(rc,re,msg,sizeof msg);free(re);rt_err("regex: bad pattern '%s': %s\n",pattern,msg);}
 return re;}

static Value regex_module(const char*fn,Value*a,int c){
 if(c<2||a[0].t!=V_STR||a[1].t!=V_STR)rt_err("regex.%s expects a pattern and text, both strings\n",fn);
 const char*pat=cstr(a[0]);const char*text=cstr(a[1]);
 regex_t*re=regex_compile(pat);
 regmatch_t m[1];
 Value out;

 if(!strcmp(fn,"matches")&&c==2){
  out=boolean(regexec(re,text,0,NULL,0)==0);
 }
 else if(!strcmp(fn,"find")&&c==2){
  out=regexec(re,text,1,m,0)==0?str(strndup(text+m[0].rm_so,(size_t)(m[0].rm_eo-m[0].rm_so))):nul();
 }
 else if(!strcmp(fn,"find_all")&&c==2){
  out=list_value();
  const char*pos=text;int flags=0;
  while(regexec(re,pos,1,m,flags)==0){
   if(m[0].rm_eo==m[0].rm_so){
    if(!pos[m[0].rm_eo]&&m[0].rm_so>0)break;
    if(!pos[m[0].rm_so])break;
    pos+=m[0].rm_so+1;
   }else{
    list_push(out.list,str(strndup(pos+m[0].rm_so,(size_t)(m[0].rm_eo-m[0].rm_so))));
    pos+=m[0].rm_eo;
   }
   flags=REG_NOTBOL;
   if(!*pos)break;
  }
 }
 else if(!strcmp(fn,"replace")&&c==3&&a[2].t==V_STR){
  Buf b={0};const char*pos=text;int flags=0;
  while(regexec(re,pos,1,m,flags)==0){
   buf_add(&b,pos,(size_t)m[0].rm_so);
   buf_str(&b,cstr(a[2]));
   if(m[0].rm_eo==m[0].rm_so){
    if(!pos[m[0].rm_so]){pos+=m[0].rm_so;break;}
    buf_add(&b,pos+m[0].rm_so,1);
    pos+=m[0].rm_so+1;
   }else pos+=m[0].rm_eo;
   flags=REG_NOTBOL;
  }
  buf_str(&b,pos);
  out=str(b.s?b.s:"");
 }
 else if(!strcmp(fn,"split")&&c==2){
  out=list_value();
  const char*pos=text;int flags=0;
  while(regexec(re,pos,1,m,flags)==0&&m[0].rm_eo>m[0].rm_so){
   list_push(out.list,str(strndup(pos,(size_t)m[0].rm_so)));
   pos+=m[0].rm_eo;flags=REG_NOTBOL;
   if(!*pos)break;
  }
  list_push(out.list,str(pos));
 }
 else{regfree(re);free(re);rt_err("regex.%s is not a function that takes these arguments\n",fn);exit(3);}
 regfree(re);free(re);
 return out;}
