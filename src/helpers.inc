static bool values_equal(Value a,Value b){if(a.t!=b.t)return false;switch(a.t){case V_NUM:return a.n==b.n;case V_BOOL:return a.b==b.b;case V_STR:return str_eq(a,b);case V_NULL:return true;
 case V_LIST:{if(a.list->len!=b.list->len)return false;for(size_t i=0;i<a.list->len;i++)if(!values_equal(a.list->items[i],b.list->items[i]))return false;return true;}
 case V_DICT:{if(a.dict->len!=b.dict->len)return false;for(size_t i=0;i<a.dict->len;i++){long j=dict_find(b.dict,a.dict->keys[i]);if(j<0||!values_equal(a.dict->values[i],b.dict->values[j]))return false;}return true;}
 default:return a.f==b.f&&a.ptr==b.ptr&&a.list==b.list&&a.dict==b.dict;}}
static Value text_of(Value v){return v.t==V_STR?v:str(stringify(v));}
static Value str_concat(Value a,Value b){
 a=text_of(a);b=text_of(b);
 size_t total=(size_t)a.len+b.len;
 if(total>UINT32_MAX){fputs("Kiln: text is too large\n",stderr);exit(3);}
 SBuf*h=(SBuf*)a.s-1;
 /* In place: a ends where the buffer's committed text ends, so no other value sees these bytes. */
 if(a.len==h->used&&total<=h->cap){size_t old=h->used;memcpy(a.s+a.len,b.s,b.len);h->used=total;a.s[total]=0;if(h->ascii==old)h->ascii=old+leading_ascii(b.s,b.len);return(Value){.t=V_STR,.len=(uint32_t)total,.s=a.s};}
 size_t cap=total<8?16:total*2;
 SBuf*nh=malloc(sizeof(SBuf)+cap+1);nh->used=total;nh->cap=cap;char*d=(char*)(nh+1);
 memcpy(d,a.s,a.len);memcpy(d+a.len,b.s,b.len);d[total]=0;nh->ascii=leading_ascii(d,total);
 return(Value){.t=V_STR,.len=(uint32_t)total,.s=d};}
static void need_numbers(Value a,Value b,const char*op){if(a.t!=V_NUM||b.t!=V_NUM)rt_err("operator %s needs numbers\n",op);}
static int compare_values(const void*x,const void*y){const Value*a=x,*b=y;if(a->t==V_STR)return str_cmp(*a,*b);return (a->n>b->n)-(a->n<b->n);}
static char* replace_all(const char*s,const char*old,const char*nw){size_t ol=strlen(old),nl=strlen(nw);if(ol==0)return strdup(s);size_t count=0;for(const char*p=s;(p=strstr(p,old))!=NULL;p+=ol)count++;char*out=malloc(strlen(s)-count*ol+count*nl+1);char*w=out;const char*p=s;const char*q;while((q=strstr(p,old))!=NULL){size_t k=(size_t)(q-p);memcpy(w,p,k);w+=k;memcpy(w,nw,nl);w+=nl;p=q+ol;}strcpy(w,p);return out;}
typedef struct {char*s;size_t n,cap;} Buf;
static void buf_add(Buf*b,const char*x,size_t k){
 if(k==0){if(!b->s){b->s=malloc(1);if(!b->s){fputs("Kiln: out of memory\n",stderr);exit(3);}b->s[0]=0;b->cap=1;}return;}
 if(k>SIZE_MAX-b->n-1){fputs("Kiln: buffer is too large\n",stderr);exit(3);}
 size_t need=b->n+k+1;
 if(need>b->cap){size_t cap=need<=SIZE_MAX/2?need*2:need;char*next=realloc(b->s,cap);if(!next){fputs("Kiln: out of memory\n",stderr);exit(3);}b->s=next;b->cap=cap;}
 memcpy(b->s+b->n,x,k);b->n+=k;b->s[b->n]=0;
}
static void buf_str(Buf*b,const char*x){buf_add(b,x,strlen(x));}
static void json_string(Buf*b,const char*s){buf_add(b,"\"",1);for(const char*p=s;*p;p++){switch(*p){case '"':buf_str(b,"\\\"");break;case '\\':buf_str(b,"\\\\");break;case '\n':buf_str(b,"\n");break;case '\t':buf_str(b,"\\t");break;case '\r':buf_str(b,"\\r");break;default:buf_add(b,p,1);}}buf_add(b,"\"",1);}
static void json_encode(Buf*b,Value v){switch(v.t){case V_NULL:buf_str(b,"null");break;case V_BOOL:buf_str(b,v.b?"true":"false");break;case V_NUM:{char tmp[64];if(v.n==floor(v.n)&&fabs(v.n)<1e15)snprintf(tmp,sizeof tmp,"%.0f",v.n);else{snprintf(tmp,sizeof tmp,"%.15g",v.n);if(strtod(tmp,NULL)!=v.n)snprintf(tmp,sizeof tmp,"%.17g",v.n);}buf_str(b,tmp);break;}case V_STR:json_string(b,cstr(v));break;case V_LIST:buf_add(b,"[",1);for(size_t i=0;i<v.list->len;i++){if(i)buf_add(b,",",1);json_encode(b,v.list->items[i]);}buf_add(b,"]",1);break;case V_DICT:buf_add(b,"{",1);for(size_t i=0;i<v.dict->len;i++){if(i)buf_add(b,",",1);json_string(b,v.dict->keys[i]);buf_add(b,":",1);json_encode(b,v.dict->values[i]);}buf_add(b,"}",1);break;default:rt_err("json.encode cannot encode this value\n");exit(3);}}
static void jfail(void){rt_err("json.decode: invalid JSON text\n");exit(3);}
static void jskip(const char**p){while(**p==' '||**p=='\n'||**p=='\t'||**p=='\r')(*p)++;}
static Value jparse(const char**p){jskip(p);char c=**p;
 if(c=='{'){(*p)++;Value v=dict_value();jskip(p);if(**p=='}'){(*p)++;return v;}for(;;){jskip(p);if(**p!='"')jfail();Value k=jparse(p);jskip(p);if(**p!=':')jfail();(*p)++;Value val=jparse(p);dict_set(v.dict,cstr(k),val);jskip(p);if(**p==','){(*p)++;continue;}if(**p=='}'){(*p)++;return v;}jfail();}}
 if(c=='['){(*p)++;Value v=list_value();jskip(p);if(**p==']'){(*p)++;return v;}for(;;){list_push(v.list,jparse(p));jskip(p);if(**p==','){(*p)++;continue;}if(**p==']'){(*p)++;return v;}jfail();}}
 if(c=='"'){(*p)++;Buf b={0};while(**p&&**p!='"'){if(**p=='\\'){(*p)++;switch(**p){case 'n':buf_add(&b,"\n",1);break;case 't':buf_add(&b,"\t",1);break;case 'r':buf_add(&b,"\r",1);break;case '"':case '\\':case '/':buf_add(&b,*p,1);break;default:jfail();}(*p)++;}else{buf_add(&b,*p,1);(*p)++;}}if(**p!='"')jfail();(*p)++;return str(b.s?b.s:"");}
 if(strncmp(*p,"true",4)==0){*p+=4;return boolean(true);}
 if(strncmp(*p,"false",5)==0){*p+=5;return boolean(false);}
 if(strncmp(*p,"null",4)==0){*p+=4;return nul();}
 if(c=='-'||isdigit((unsigned char)c)){char*end;double d=strtod(*p,&end);if(end==*p)jfail();*p=end;return num(d);}
 jfail();return nul();}
static Value json_module(const char*fn,Value*a,int c){
 if(!strcmp(fn,"encode")){if(c!=1){rt_err("json.encode expects one value\n");exit(3);}Buf b={0};json_encode(&b,a[0]);return str(b.s?b.s:"");}
 if(!strcmp(fn,"decode")){if(c!=1||a[0].t!=V_STR){rt_err("json.decode expects one string\n");exit(3);}const char*p=cstr(a[0]);Value v=jparse(&p);jskip(&p);if(*p)jfail();return v;}
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
