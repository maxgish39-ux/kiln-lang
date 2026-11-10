#define KILN_MAX_CALL_DEPTH 1500
static Value run_body(Node*body,Env*e);
static bool contains_closure(Node*n);
static Value call(Value fn,Value*args,int argc,Env*e){
 if(fn.t==V_CLASS){Class*c=fn.ptr;Value self=instance_new(c);Node*init=find_method(c,"init");if(init)call(bind_method(c,init,self),args,argc,e);else if(argc)rt_err("class %s takes no arguments\n",c->name);return self;}int saved_line=g_line;if(fn.t!=V_FUNC){rt_err("value is not callable\n");exit(3);}Func*f=fn.f;int required=f->count;while(required>0&&f->defaults&&f->defaults[required-1])required--;
 if(argc<required||argc>f->count){if(required==f->count)rt_err("function expected %d arguments, got %d\n",f->count,argc);else rt_err("function expects %d to %d arguments, got %d\n",required,f->count,argc);}
 if(call_depth>=KILN_MAX_CALL_DEPTH){rt_err("maximum recursion depth of %d exceeded\n",KILN_MAX_CALL_DEPTH);exit(3);}call_depth++;Env*local=env_new(f->closure?f->closure:e);
 for(int i=0;i<f->count;i++){Value v;if(i<argc)v=args[i];else v=eval(f->defaults[i],f->closure?f->closure:e);define(local,f->names[i],v);}bool old=returning;int old_floor=loop_floor;loop_floor=loop_depth;returning=false;Value r=run_body(f->body,local);loop_floor=old_floor;g_line=saved_line;call_depth--;if(!f->body->captures)f->body->captures=contains_closure(f->body)?2:1;if(f->body->captures==1)env_release(local);if(returning){r=retvalue;returning=old;}return r;}
#define ARITY(z) if(c!=(z)){rt_err("%s.%s expects %d arguments\n",m,fn,z);exit(3);}
