/* HTTP and HTTPS client.
 *
 * http:// requests use a built-in TCP client.
 * https:// requests run the system `curl` command (no TLS library is needed to
 * build Kiln). Arguments are passed as an argument list, never through a shell.
 * Responses are parsed the same way for both.
 */
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <strings.h>

typedef struct { char**lines; size_t n, cap; } HeaderList;

static void hl_add(HeaderList*h,char*line){
 if(h->n==h->cap){h->cap=h->cap?h->cap*2:8;h->lines=realloc(h->lines,h->cap*sizeof(char*));}
 h->lines[h->n++]=line;}

static void http_check_text(const char*s){
 for(const char*c=s;*c;c++)if(*c=='\r'||*c=='\n')rt_err("http: header names and values cannot contain line breaks\n");}

static void http_collect_headers(Value headers,HeaderList*hl){
 if(headers.t!=V_DICT)return;
 for(size_t i=0;i<headers.dict->len;i++){
  char*k=headers.dict->keys[i];char*v=stringify(headers.dict->values[i]);
  http_check_text(k);http_check_text(v);
  size_t n=strlen(k)+strlen(v)+3;char*line=malloc(n);snprintf(line,n,"%s: %s",k,v);
  hl_add(hl,line);
 }}

static bool http_split_url(const char*url,char*host,size_t hn,char*port,size_t pn,char*path,size_t pathn){
 if(strncmp(url,"http://",7)!=0)return false;
 const char*p=url+7;const char*slash=strchr(p,'/');size_t hl=slash?(size_t)(slash-p):strlen(p);
 if(hl==0||hl>=hn)return false;
 memcpy(host,p,hl);host[hl]=0;
 char*colon=strchr(host,':');
 if(colon){*colon=0;snprintf(port,pn,"%s",colon+1);}else snprintf(port,pn,"80");
 if(!host[0])return false;
 snprintf(path,pathn,"%s",slash?slash:"/");
 return true;}

static void http_dechunk(const Buf*raw,Buf*out){
 const char*p=raw->s;const char*end=raw->s+raw->n;
 while(p<end){
  char*e;long sz=strtol(p,&e,16);
  if(e==p)break;
  p=strstr(e,"\r\n");if(!p)break;p+=2;
  if(sz<=0)break;
  if(p+sz>end)sz=end-p;
  buf_add(out,p,(size_t)sz);
  p+=sz;
  if(p+2<=end)p+=2;
 }}

/* Send a plain-HTTP request over a TCP socket and return the raw response. */
static Buf http_socket_exchange(const char*method,const char*url,const char*body,HeaderList*hl,const char**host_out){
 char host[256],port[16],path[2048];
 if(!http_split_url(url,host,sizeof host,port,sizeof port,path,sizeof path))rt_err("http: URL must start with http:// or https:// and include a host: %s\n",url);
 *host_out=strdup(host);
 struct addrinfo hints;memset(&hints,0,sizeof hints);hints.ai_family=AF_UNSPEC;hints.ai_socktype=SOCK_STREAM;
 struct addrinfo*res=NULL;
 int gai=getaddrinfo(host,port,&hints,&res);
 if(gai!=0)rt_err("http: cannot resolve %s: %s\n",host,gai_strerror(gai));
 int fd=-1;
 for(struct addrinfo*ai=res;ai;ai=ai->ai_next){
  fd=socket(ai->ai_family,ai->ai_socktype,ai->ai_protocol);if(fd<0)continue;
  struct timeval tv={15,0};
  setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof tv);
  setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof tv);
  if(connect(fd,ai->ai_addr,ai->ai_addrlen)==0)break;
  close(fd);fd=-1;
 }
 freeaddrinfo(res);
 if(fd<0)rt_err("http: cannot connect to %s port %s\n",host,port);

 Buf req={0};char line[4200];
 snprintf(line,sizeof line,"%s %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\nUser-Agent: Kiln\r\n",method,path,host);
 buf_str(&req,line);
 for(size_t i=0;i<hl->n;i++){buf_str(&req,hl->lines[i]);buf_str(&req,"\r\n");}
 if(body){
  bool has_type=false;
  for(size_t i=0;i<hl->n;i++)if(!strncasecmp(hl->lines[i],"Content-Type:",13))has_type=true;
  if(!has_type)buf_str(&req,"Content-Type: text/plain; charset=utf-8\r\n");
  snprintf(line,sizeof line,"Content-Length: %zu\r\n",strlen(body));buf_str(&req,line);
 }
 buf_str(&req,"\r\n");
 if(body)buf_str(&req,body);

 size_t sent=0;
 while(sent<req.n){
  ssize_t w=send(fd,req.s+sent,req.n-sent,MSG_NOSIGNAL);
  if(w<=0){close(fd);rt_err("http: could not send the request to %s\n",host);}
  sent+=(size_t)w;
 }
 Buf resp={0};char chunk[8192];
 for(;;){
  ssize_t r=recv(fd,chunk,sizeof chunk,0);
  if(r<0&&errno==EINTR)continue;
  if(r<=0)break;
  if(resp.n+(size_t)r>64u*1024u*1024u){close(fd);rt_err("http: response from %s is larger than 64 MB\n",host);}
  buf_add(&resp,chunk,(size_t)r);
 }
 close(fd);
 return resp;}

/* Run curl for an https:// request. Its output (headers and body) is the response. */
static Buf http_curl_exchange(const char*method,const char*url,const char*body,HeaderList*hl){
 char*tmp=NULL;
 if(body){
  char tmpl[]="/tmp/kiln-body-XXXXXX";
  int tfd=mkstemp(tmpl);
  if(tfd<0)rt_err("https: could not create a temporary file for the request body\n");
  size_t bl=strlen(body);
  if(write(tfd,body,bl)!=(ssize_t)bl){close(tfd);unlink(tmpl);rt_err("https: could not write the request body\n");}
  close(tfd);
  tmp=strdup(tmpl);
 }
 size_t cap=16+2*hl->n+(body?4:0);
 char**argv=calloc(cap,sizeof(char*));size_t ac=0;
 argv[ac++]="curl";argv[ac++]="-sS";argv[ac++]="-i";argv[ac++]="--max-time";argv[ac++]="30";
 argv[ac++]="-A";argv[ac++]="Kiln";
 argv[ac++]="-X";argv[ac++]=(char*)method;
 for(size_t i=0;i<hl->n;i++){argv[ac++]="-H";argv[ac++]=hl->lines[i];}
 if(body){
  bool has_type=false;
  for(size_t i=0;i<hl->n;i++)if(!strncasecmp(hl->lines[i],"Content-Type:",13))has_type=true;
  if(!has_type){argv[ac++]="-H";argv[ac++]="Content-Type: text/plain; charset=utf-8";}
  size_t n=strlen(tmp)+2;char*arg=malloc(n);snprintf(arg,n,"@%s",tmp);
  argv[ac++]="--data-binary";argv[ac++]=arg;
 }
 argv[ac++]=(char*)url;
 argv[ac]=NULL;

 int pipefd[2];
 if(pipe(pipefd)!=0)rt_err("https: could not start curl\n");
 pid_t pid=fork();
 if(pid<0)rt_err("https: could not start curl\n");
 if(pid==0){
  dup2(pipefd[1],1);close(pipefd[0]);close(pipefd[1]);
  execvp("curl",argv);
  _exit(127);
 }
 close(pipefd[1]);
 Buf out={0};char chunk[8192];
 for(;;){
  ssize_t r=read(pipefd[0],chunk,sizeof chunk);
  if(r<0&&errno==EINTR)continue;
  if(r<=0)break;
  buf_add(&out,chunk,(size_t)r);
 }
 close(pipefd[0]);
 int status=0;
 while(waitpid(pid,&status,0)<0&&errno==EINTR){}
 if(tmp){unlink(tmp);free(tmp);}
 free(argv);
 if(WIFEXITED(status)&&WEXITSTATUS(status)==127)rt_err("https: the curl command was not found; install curl to use https:// addresses\n");
 if(!WIFEXITED(status)||WEXITSTATUS(status)!=0)rt_err("https: request failed (curl exit code %d); see the message above\n",WIFEXITED(status)?WEXITSTATUS(status):-1);
 return out;}

/* Parse a raw HTTP response (status line, headers, body) into a dictionary. */
static Value http_parse(Buf*resp,const char*host,bool allow_dechunk){
 if(resp->n==0)rt_err("http: no response from %s\n",host);
 const char*p=resp->s;const char*sep;int status=0;
 for(;;){
  sep=strstr(p,"\r\n\r\n");
  if(!sep)rt_err("http: malformed response from %s\n",host);
  if(sscanf(p,"HTTP/%*s %d",&status)!=1)rt_err("http: malformed status line from %s\n",host);
  if(status>=100&&status<200){p=sep+4;continue;}   /* skip 1xx interim replies */
  break;
 }
 Value hv=dict_value();
 const char*eol=strstr(p,"\r\n");
 const char*q=(eol&&eol<sep)?eol+2:sep;
 while(q<sep){
  const char*e=strstr(q,"\r\n");if(!e||e>sep)e=sep;
  const char*colon=memchr(q,':',(size_t)(e-q));
  if(colon){
   size_t kl=(size_t)(colon-q);
   char*key=malloc(kl+1);
   for(size_t i=0;i<kl;i++)key[i]=(char)tolower((unsigned char)q[i]);
   key[kl]=0;
   const char*vs=colon+1;while(vs<e&&(*vs==' '||*vs=='\t'))vs++;
   char*val=malloc((size_t)(e-vs)+1);memcpy(val,vs,(size_t)(e-vs));val[e-vs]=0;
   dict_set(hv.dict,key,str(val));
  }
  if(e>=sep)break;
  q=e+2;
 }
 Buf rest={0};
 size_t body_offset=(size_t)((sep+4)-resp->s);
 if(body_offset>resp->n)rt_err("http: malformed response body from %s\n",host);
 size_t body_len=resp->n-body_offset;
 buf_add(&rest,resp->s+body_offset,body_len);
 long ti=dict_find(hv.dict,"transfer-encoding");
 bool chunked=ti>=0&&strstr(cstr(hv.dict->values[ti]),"chunked");
 Buf text={0};
 if(allow_dechunk&&chunked)http_dechunk(&rest,&text);
 else buf_add(&text,rest.s?rest.s:"",rest.n);

 Value out=dict_value();
 dict_set(out.dict,"status",num((double)status));
 dict_set(out.dict,"ok",boolean(status>=200&&status<300));
 dict_set(out.dict,"body",str(text.s?text.s:""));
 dict_set(out.dict,"headers",hv);
 return out;}

static Value http_request(const char*method,const char*url,const char*body,Value headers){
 HeaderList hl={0};
 http_collect_headers(headers,&hl);
 if(strncmp(url,"https://",8)==0){
  Buf resp=http_curl_exchange(method,url,body,&hl);
  return http_parse(&resp,url+8,false);   /* curl has already decoded chunked bodies */
 }
 if(strncmp(url,"http://",7)==0){
  const char*host=NULL;
  Buf resp=http_socket_exchange(method,url,body,&hl,&host);
  return http_parse(&resp,host,true);
 }
 rt_err("http: URL must start with http:// or https://: %s\n",url);
 return nul();}

static Value http_module(const char*fn,Value*a,int c){
 if(!strcmp(fn,"get")){
  if(c!=1||a[0].t!=V_STR)rt_err("http.get expects one URL string\n");
  return http_request("GET",cstr(a[0]),NULL,nul());}
 if(!strcmp(fn,"post")){
  if(c!=2||a[0].t!=V_STR||a[1].t!=V_STR)rt_err("http.post expects a URL string and a body string\n");
  return http_request("POST",cstr(a[0]),cstr(a[1]),nul());}
 if(!strcmp(fn,"request")){
  if(c!=4||a[0].t!=V_STR||a[1].t!=V_STR||(a[2].t!=V_STR&&a[2].t!=V_NULL)||(a[3].t!=V_DICT&&a[3].t!=V_NULL))rt_err("http.request expects method, URL, body (text or null) and headers (dictionary or null)\n");
  return http_request(cstr(a[0]),cstr(a[1]),a[2].t==V_STR?cstr(a[2]):NULL,a[3]);}
 if(!strcmp(fn,"get_json")){
  if(c!=1||a[0].t!=V_STR)rt_err("http.get_json expects one URL string\n");
  Value r=http_request("GET",cstr(a[0]),NULL,nul());
  Value ok=r.dict->values[dict_find(r.dict,"ok")];
  if(!ok.b)rt_err("http.get_json: server replied with status %s\n",stringify(r.dict->values[dict_find(r.dict,"status")]));
  const char*p=cstr(r.dict->values[dict_find(r.dict,"body")]);
  Value v=jparse(&p);jskip(&p);if(*p)jfail();
  return v;}
 rt_err("http.%s is not a function\n",fn);exit(3);}
