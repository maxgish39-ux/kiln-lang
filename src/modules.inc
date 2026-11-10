static Value module_call(const char*m,const char*fn,Value*a,int c){
 if(!strcmp(m,"time"))return time_module(fn,a,c);
 if(!strcmp(m,"path"))return path_module(fn,a,c);
 if(!strcmp(m,"json"))return json_module(fn,a,c);
 if(!strcmp(m,"http"))return http_module(fn,a,c);
 if(!strcmp(m,"regex"))return regex_module(fn,a,c);
 if(!strcmp(m,"math")&&!strcmp(fn,"random")){static bool seeded=false;if(!seeded){srand((unsigned)time(NULL));seeded=true;}double unit=(double)rand()/((double)RAND_MAX+1.0);if(c==0)return num(unit);if(c==2){double lo=ceil(a[0].n),hi=floor(a[1].n);if(hi<lo){rt_err("math.random needs low <= high\n");exit(3);}return num(lo+floor(unit*(hi-lo+1)));}rt_err("math.random expects no arguments or (low, high)\n");exit(3);}
if(!strcmp(m,"math")){if(!strcmp(fn,"clamp")){ARITY(3)if(a[0].t!=V_NUM||a[1].t!=V_NUM||a[2].t!=V_NUM||a[1].n>a[2].n){rt_err("math.clamp needs numbers with low <= high\n");exit(3);}double x=a[0].n;return num(x<a[1].n?a[1].n:(x>a[2].n?a[2].n:x));}if(!strcmp(fn,"log")||!strcmp(fn,"exp")){ARITY(1)if(a[0].t!=V_NUM){rt_err("math.%s needs a number\n",fn);exit(3);}return num(!strcmp(fn,"log")?log(a[0].n):exp(a[0].n));}if(!strcmp(fn,"pi")){if(c!=0){rt_err("math.pi expects 0 arguments\n");exit(3);}return num(acos(-1));} if(!strcmp(fn,"pow")||!strcmp(fn,"min")||!strcmp(fn,"max")){if(c!=2){rt_err("math.%s expects 2 arguments\n",fn);exit(3);}if(!strcmp(fn,"pow"))return num(pow(a[0].n,a[1].n));return num(!strcmp(fn,"min")?fmin(a[0].n,a[1].n):fmax(a[0].n,a[1].n));} ARITY(1) if(!strcmp(fn,"sqrt"))return num(sqrt(a[0].n));if(!strcmp(fn,"abs"))return num(fabs(a[0].n));if(!strcmp(fn,"floor"))return num(floor(a[0].n));if(!strcmp(fn,"ceil"))return num(ceil(a[0].n));if(!strcmp(fn,"sin"))return num(sin(a[0].n));if(!strcmp(fn,"cos"))return num(cos(a[0].n));if(!strcmp(fn,"tan"))return num(tan(a[0].n));if(!strcmp(fn,"round"))return num(round(a[0].n));}
 if(!strcmp(m,"string")){if(c<1){rt_err("string.%s expects arguments\n",fn);exit(3);}char*s=stringify(a[0]);if(!strcmp(fn,"upper")||!strcmp(fn,"lower")){ARITY(1)for(char*p=s;*p;p++)*p=(char)(!strcmp(fn,"upper")?toupper((unsigned char)*p):tolower((unsigned char)*p));return str(s);}if(!strcmp(fn,"length")){ARITY(1)return num(text_len(text_of(a[0])));}if(!strcmp(fn,"contains")){if(c!=2){rt_err("string.contains expects 2 arguments\n");exit(3);}return boolean(strstr(cstr(a[0]),cstr(a[1]))!=NULL);}if(!strcmp(fn,"trim")){ARITY(1)char*p=s;while(isspace((unsigned char)*p))p++;char*end=p+strlen(p);while(end>p&&isspace((unsigned char)end[-1]))end--;return str(slice(p,(size_t)(end-p)));}if(!strcmp(fn,"starts_with")||!strcmp(fn,"ends_with")){ARITY(2)char*t=stringify(a[1]);size_t sl=strlen(s),tl=strlen(t);bool hit=!strcmp(fn,"starts_with")?(tl<=sl&&strncmp(s,t,tl)==0):(tl<=sl&&strcmp(s+sl-tl,t)==0);return boolean(hit);}if(!strcmp(fn,"split")){if(c<2||c>3){rt_err("string.split expects a separator and an optional maximum number of splits\n");exit(3);}if(c==3&&a[2].t!=V_NUM){rt_err("string.split maximum splits must be a number\n");exit(3);}long max_splits=c==3?(long)a[2].n:-1;char*t=stringify(a[1]);Value v=list_value();size_t tl=strlen(t);if(tl==0){for(size_t i=0,n=strlen(s);i<n;){if(max_splits>=0&&(long)v.list->len>=max_splits){list_push(v.list,str(s+i));break;}size_t w=utf8_char_width(s,i,n);list_push(v.list,strv_n(s+i,w));i+=w;}return v;}const char*p=s;const char*q;long splits=0;while((max_splits<0||splits<max_splits)&&(q=strstr(p,t))!=NULL){list_push(v.list,str(slice(p,(size_t)(q-p))));p=q+tl;splits++;}list_push(v.list,str(p));return v;}if(!strcmp(fn,"pad_left")||!strcmp(fn,"pad_right")){if(c<2||c>3||a[1].t!=V_NUM||a[1].n<0){rt_err("string.%s expects a width and an optional fill character\n",fn);exit(3);}size_t w=(size_t)a[1].n,len=utf8_count(s,strlen(s));char*f=" ";if(c==3){f=stringify(a[2]);if(utf8_count(f,strlen(f))!=1){rt_err("string.%s fill must be one character\n",fn);exit(3);}}if(len>=w)return str(s);Buf b={0};size_t extra=w-len;if(!strcmp(fn,"pad_left")){for(size_t k=0;k<extra;k++)buf_str(&b,f);buf_str(&b,s);}else{buf_str(&b,s);for(size_t k=0;k<extra;k++)buf_str(&b,f);}return str(b.s);}if(!strcmp(fn,"repeat")){ARITY(2)if(a[1].t!=V_NUM||a[1].n<0){rt_err("string.repeat expects a non-negative count\n");exit(3);}Buf b={0};for(long k=0;k<(long)a[1].n;k++)buf_str(&b,s);return str(b.s?b.s:"");}if(!strcmp(fn,"index_of")){ARITY(2)Value h=text_of(a[0]),nd=text_of(a[1]);long off=str_find(h,nd);return num(off<0?-1:(double)utf8_count(h.s,(size_t)off));}if(!strcmp(fn,"replace")){ARITY(3)char*t=stringify(a[1]);char*u=stringify(a[2]);return str(replace_all(s,t,u));}}
 if(!strcmp(m,"fs")){if(!strcmp(fn,"lines")){ARITY(1)FILE*f=fopen(cstr(a[0]),"r");if(!f){rt_err("fs.lines cannot open %s\n",cstr(a[0]));exit(3);}Value v=list_value();char*line=NULL;size_t cap=0;ssize_t len;while((len=getline(&line,&cap,f))!=-1){if(len&&line[len-1]=='\n')line[--len]=0;if(len&&line[len-1]=='\r')line[len-1]=0;list_push(v.list,str(line));}free(line);fclose(f);return v;}if(!strcmp(fn,"exists")){ARITY(1)struct stat st;return boolean(stat(cstr(a[0]),&st)==0);}if(!strcmp(fn,"read")){ARITY(1)FILE*f=fopen(cstr(a[0]),"rb");if(!f)return str("");fseek(f,0,SEEK_END);long z=ftell(f);rewind(f);char*s=calloc((size_t)z+1,1);size_t got=fread(s,1,(size_t)z,f);s[got]=0;fclose(f);return str(s);}if(!strcmp(fn,"write")||!strcmp(fn,"append")){ARITY(2)FILE*f=fopen(cstr(a[0]),!strcmp(fn,"write")?"wb":"ab");if(!f)return boolean(false);char*s=stringify(a[1]);fputs(s,f);fclose(f);return boolean(true);}if(!strcmp(fn,"remove")){ARITY(1)return boolean(remove(cstr(a[0]))==0);}if(!strcmp(fn,"mkdir")){ARITY(1)return boolean(mkdir(cstr(a[0]),0775)==0);}if(!strcmp(fn,"is_dir")){ARITY(1)struct stat st;return boolean(stat(cstr(a[0]),&st)==0&&S_ISDIR(st.st_mode));}if(!strcmp(fn,"size")){ARITY(1)struct stat st;if(stat(cstr(a[0]),&st)!=0)return num(-1);return num((double)st.st_size);}if(!strcmp(fn,"copy")){ARITY(2)FILE*in=fopen(cstr(a[0]),"rb");if(!in)return boolean(false);FILE*out=fopen(cstr(a[1]),"wb");if(!out){fclose(in);return boolean(false);}char buf[16384];size_t n;bool ok=true;while((n=fread(buf,1,sizeof buf,in))>0){if(fwrite(buf,1,n,out)!=n){ok=false;break;}}if(ferror(in))ok=false;if(fclose(in)!=0)ok=false;if(fclose(out)!=0)ok=false;if(!ok)remove(cstr(a[1]));return boolean(ok);} }
  if(!strcmp(m,"gui")){
#ifdef KILN_WITH_RAYLIB
    if(!strcmp(fn,"open")){ARITY(3) InitWindow((int)a[0].n,(int)a[1].n,cstr(a[2]));return nul();}
    if(!strcmp(fn,"close")){ARITY(0) CloseWindow();return nul();}
    if(!strcmp(fn,"should_close")){ARITY(0) return boolean(WindowShouldClose());}
    if(!strcmp(fn,"begin")){ARITY(0) BeginDrawing();return nul();}
    if(!strcmp(fn,"end")){ARITY(0) EndDrawing();return nul();}
    if(!strcmp(fn,"clear")){ARITY(3) ClearBackground(kiln_color(a));return nul();}
    if(!strcmp(fn,"text")){ARITY(7) DrawText(cstr(a[0]),(int)a[1].n,(int)a[2].n,(int)a[3].n,kiln_color(a+4));return nul();}
    if(!strcmp(fn,"rect")){ARITY(7) DrawRectangle((int)a[0].n,(int)a[1].n,(int)a[2].n,(int)a[3].n,kiln_color(a+4));return nul();}
    if(!strcmp(fn,"circle")){ARITY(6) DrawCircle((int)a[0].n,(int)a[1].n,(float)a[2].n,kiln_color(a+3));return nul();}
    if(!strcmp(fn,"key_down")||!strcmp(fn,"key_pressed")){ARITY(1) int key=kiln_key_code(cstr(a[0]));if(key<0)return boolean(false);return boolean(!strcmp(fn,"key_down")?IsKeyDown(key):IsKeyPressed(key));}
    if(!strcmp(fn,"mouse_x")){ARITY(0) return num(GetMouseX());}
    if(!strcmp(fn,"mouse_y")){ARITY(0) return num(GetMouseY());}
    if(!strcmp(fn,"mouse_pressed")){ARITY(1) int button=kiln_mouse_code(cstr(a[0]));return boolean(button>=0&&IsMouseButtonPressed(button));}
    if(!strcmp(fn,"fps")){ARITY(1) SetTargetFPS((int)a[0].n);return nul();}
    if(!strcmp(fn,"line")){ARITY(7) DrawLine((int)a[0].n,(int)a[1].n,(int)a[2].n,(int)a[3].n,kiln_color(a+4));return nul();}
    if(!strcmp(fn,"rect_outline")){ARITY(7) DrawRectangleLines((int)a[0].n,(int)a[1].n,(int)a[2].n,(int)a[3].n,kiln_color(a+4));return nul();}
    if(!strcmp(fn,"measure_text")){ARITY(2) return num(MeasureText(cstr(a[0]),(int)a[1].n));}
    if(!strcmp(fn,"window_width")){ARITY(0) return num(GetScreenWidth());}
    if(!strcmp(fn,"window_height")){ARITY(0) return num(GetScreenHeight());}
    if(!strcmp(fn,"mouse_wheel")){ARITY(0) return num(GetMouseWheelMove());}
    if(!strcmp(fn,"mouse_down")){ARITY(1) int button=kiln_mouse_code(cstr(a[0]));return boolean(button>=0&&IsMouseButtonDown(button));}
    if(!strcmp(fn,"set_title")){ARITY(1) SetWindowTitle(cstr(a[0]));return nul();}
    if(!strcmp(fn,"toggle_fullscreen")){ARITY(0) ToggleFullscreen();return nul();}
    if(!strcmp(fn,"button")){
      ARITY(11)
      int x=(int)a[1].n,y=(int)a[2].n,w=(int)a[3].n,h=(int)a[4].n;
      int mx=GetMouseX(),my=GetMouseY();
      bool hover=mx>=x&&mx<=x+w&&my>=y&&my<=y+h;
      Color bg=kiln_color(a+5),fg=kiln_color(a+8);
      if(hover){bg.r=(unsigned char)(bg.r>230?255:bg.r+25);bg.g=(unsigned char)(bg.g>230?255:bg.g+25);bg.b=(unsigned char)(bg.b>230?255:bg.b+25);}
      DrawRectangle(x,y,w,h,bg);
      int tw=MeasureText(cstr(a[0]),20);
      DrawText(cstr(a[0]),x+(w-tw)/2,y+(h-20)/2,20,fg);
      return boolean(hover&&IsMouseButtonPressed(MOUSE_BUTTON_LEFT));
    }
#else
    rt_err("GUI module unavailable; install raylib development files and rebuild with make raylib\n");exit(3);
#endif
  }
 if(!strcmp(m,"sqlite")){
#ifdef KILN_WITH_SQLITE
   if(!strcmp(fn,"open")){ARITY(1) sqlite3*db=NULL;int rc=sqlite3_open(cstr(a[0]),&db);if(rc!=SQLITE_OK){fprintf(stderr,"Kiln sqlite error: %s\n",db?sqlite3_errmsg(db):"open failed");if(db)sqlite3_close(db);exit(3);}Value v={.t=V_DB,.ptr=db};return v;}
#else
   rt_err("SQLite module unavailable; install libsqlite3-dev and rebuild Kiln\n");exit(3);
#endif
 }
 if(!strcmp(m,"system")){if(!strcmp(fn,"cwd")){ARITY(0)char b[4096];return str(getcwd(b,sizeof b)?b:"");}if(!strcmp(fn,"env")){ARITY(1)char*s=getenv(cstr(a[0]));return str(s?s:"");}}
 rt_err("unknown function %s.%s\n",m,fn);exit(3);}
#undef ARITY
static bool member_of(Value item,Value container){
 if(container.t==V_LIST){for(size_t i=0;i<container.list->len;i++){Value x=container.list->items[i];if(x.t!=item.t)continue;if(item.t==V_NUM&&x.n==item.n)return true;if(item.t==V_STR&&str_eq(x,item))return true;if(item.t==V_BOOL&&x.b==item.b)return true;if(item.t==V_NULL)return true;}return false;}
 if(container.t==V_DICT){char*k=stringify(item);return dict_find(container.dict,k)>=0;}
 if(container.t==V_STR){if(item.t!=V_STR)rt_err("'in' needs text on the left when searching text\n");return str_find(container,item)>=0;}
 rt_err("'in' needs a list, dictionary or text on the right\n");return false;}
static char g_script_dir[4096]=".";
