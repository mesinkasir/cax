#include "template_engine.h"
#include "../utils/file_utils.h"
#include "../utils/string_utils.h"
#include "../data/data_manager.h"
#include "../frontmatter/frontmatter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void trim_inplace(char *s){
    if(!s) return;
    char *a=s; while(*a==' '||*a=='\t'||*a=='\n'||*a=='\r') a++;
    if(a!=s) memmove(s,a,strlen(a)+1);
    size_t n=strlen(s);
    while(n>0 && (s[n-1]==' '||s[n-1]=='\t'||s[n-1]=='\n'||s[n-1]=='\r')) s[--n]='\0';
}
static const char* get_nested_value(CAXObject *ctx, const char *path){
    if(!ctx||!path) return NULL;
    char buf[512]; strncpy(buf,path,511); buf[511]='\0'; trim_inplace(buf);
    if(!*buf) return NULL;
    const char *d=cax_object_get_string(ctx,buf);
    if(d) return d;
    CAXObject *cur=ctx; char *save=NULL;
    char *tok=strtok_r(buf,".",&save);
    while(tok){
        trim_inplace(tok);
        char *nxt=strtok_r(NULL,".",&save);
        if(nxt){
            CAXObject *o=(CAXObject*)cax_object_get_pointer(cur,tok);
            if(!o) return NULL;
            cur=o; tok=nxt;
        } else {
            const char *v=cax_object_get_string(cur,tok);
            if(v) return v;
            CAXObject *o=(CAXObject*)cax_object_get_pointer(cur,tok);
            if(o){
                const char *x;
                x=cax_object_get_string(o,"value"); if(x) return x;
                x=cax_object_get_string(o,"url"); if(x) return x;
                for(int i=0;i<o->count;i++) if(o->fields[i].type==CAX_TYPE_STRING) return o->fields[i].string_val;
            }
            return NULL;
        }
    }
    return NULL;
}
static const char* get_value_or_chain(CAXObject *ctx, const char *raw){
    if(!raw) return NULL;
    char work[1024]; strncpy(work,raw,1023); work[1023]='\0';
    char *p=work;
    while(p && *p){
        char *or1=strstr(p," or ");
        char *or2=strstr(p,"||");
        char *cut=NULL; int adv=0;
        if(or1 && or2){ cut = or1 < or2? or1 : or2; adv = cut==or1?4:2; }
        else if(or1){ cut=or1; adv=4; }
        else if(or2){ cut=or2; adv=2; }
        char cur[512]={0};
        if(cut){ strncpy(cur,p,cut-p); cur[cut-p]='\0'; p=cut+adv; }
        else { strcpy(cur,p); p=NULL; }
        trim_inplace(cur);
        if(*cur){
            const char *v=get_nested_value(ctx,cur);
            if(v && *v) return v;
        }
    }
    return NULL;
}
static CAXArray* resolve_array_path(CAXObject *ctx, const char *path){
    if(!ctx||!path) return NULL;
    char buf[512]; strncpy(buf,path,511); buf[511]='\0'; trim_inplace(buf);
    if(!*buf) return NULL;
    char *pipe=strchr(buf,'|'); if(pipe) *pipe='\0';
    trim_inplace(buf);
    if(strncmp(buf,"collections.",12)==0) memmove(buf,buf+12,strlen(buf+12)+1);
    CAXObject *cur=ctx; char *save=NULL;
    char *tok=strtok_r(buf,".",&save);
    while(tok){
        trim_inplace(tok);
        char *nxt=strtok_r(NULL,".",&save);
        if(nxt){
            CAXObject *o=(CAXObject*)cax_object_get_pointer(cur,tok);
            if(!o) return NULL;
            cur=o; tok=nxt;
        } else {
            CAXArray *a=(CAXArray*)cax_object_get_pointer(cur,tok);
            if(a) return a;
            CAXObject *cols=(CAXObject*)cax_object_get_pointer(ctx,"collections");
            if(cols){
                a=(CAXArray*)cax_object_get_pointer(cols,tok);
                if(a) return a;
            }
            return NULL;
        }
    }
    return NULL;
}
static int parse_limit(const char *raw){
    if(!raw) return -1;
    const char *p = strchr(raw, '|');
    if(!p) return -1;
    p++;
    while(*p && isspace((unsigned char)*p)) p++;
    if(strncmp(p, "limit", 5) != 0) return -1;
    p += 5;
    while(*p && (isspace((unsigned char)*p) || *p == ':' || *p == '(')) p++;
    char num[16] = {0}; int i = 0;
    while(*p >= '0' && *p <= '9' && i < 15) num[i++] = *p++;
    if(i == 0) return -1;
    return atoi(num);
}
static int is_truthy(CAXObject *ctx, const char *cond){
    if(!cond) return 0;
    char buf[512]; strncpy(buf,cond,511); buf[511]='\0'; trim_inplace(buf);
    char *c=buf; int neg=0;
    if(strncmp(c,"not ",4)==0){ neg=1; c+=4; trim_inplace(c); }
    char *eq=strstr(c,"==");
    if(eq){
        *eq='\0'; char *l=c, *r=eq+2; trim_inplace(l); trim_inplace(r);
        const char *lv=get_value_or_chain(ctx,l); if(!lv) lv="";
        char r2[256]; strncpy(r2,r,255); r2[255]='\0'; trim_inplace(r2);
        char *rs=r2; while(*rs=='"'||*rs=='\'') rs++;
        char *re=rs+strlen(rs)-1; while(re>rs && (*re=='"'||*re=='\'')) *re--='\0';
        int res=strcmp(lv,rs)==0; return neg?!res:res;
    }
    const char *v=get_value_or_chain(ctx,c);
    CAXArray *a=resolve_array_path(ctx,c);
    int ex=(v && *v && strcmp(v,"0")!=0 && strcmp(v,"false")!=0) || (a && a->count>0);
    return neg?!ex:ex;
}
static char* load_include_file(const char *name){
    const char *bases[]={"templates/partials/%s","templates/%s","%s",NULL};
    char p[1024];
    for(int i=0;bases[i];i++){ snprintf(p,sizeof(p),bases[i],name); char *d=cax_read_file(p); if(d) return d; }
    return NULL;
}
char *cax_template_render(const char *tpl, CAXObject *ctx){
    if(!tpl) return cax_strdup("");
    size_t cap=strlen(tpl)*2+8192;
    char *out=malloc(cap); out[0]='\0'; size_t len=0;
    const char *q=tpl;
    while(*q){
        if(q[0]=='{' && q[1]=='{'){
            q+=2; while(*q==' '||*q=='\t') q++;
            const char *e=strstr(q,"}}"); if(!e) break;
            size_t l=e-q; while(l>0 && (q[l-1]==' '||q[l-1]=='\t')) l--;
            char var[1024]; if(l>=1023) l=1023; memcpy(var,q,l); var[l]='\0'; q=e+2;
            const char *val=get_value_or_chain(ctx,var);
            if(val){ size_t vl=strlen(val); if(len+vl>=cap){ cap=cap*2+vl+1024; out=realloc(out,cap); }
                strcpy(out+len,val); len+=vl;
            }
        } else if(q[0]=='{' && q[1]=='%'){
            q+=2; while(*q==' '||*q=='\t') q++;
            const char *e=strstr(q,"%}"); if(!e) break;
            size_t tl=e-q; char tag[1024]; if(tl>=1024) tl=1023; memcpy(tag,q,tl); tag[tl]='\0'; q=e+2;
            char *t=tag; while(*t==' '||*t=='\t') t++;
            if(strncmp(t,"include ",8)==0){
                char name[512]; if(sscanf(t,"include %511s",name)==1){
                    trim_inplace(name); char *ns=name; while(*ns=='"'||*ns=='\'') ns++;
                    char *ne=ns+strlen(ns)-1; while(ne>ns && (*ne=='"'||*ne=='\'')) *ne--='\0';
                    char *pc=load_include_file(ns);
                    if(pc){ char *rp=cax_template_render(pc,ctx); free(pc);
                        if(rp){ size_t rl=strlen(rp); if(len+rl>=cap){ cap=cap*2+rl+1024; out=realloc(out,cap); }
                            strcpy(out+len,rp); len+=rl; free(rp);
                        }
                    }
                }
            } else if(strncmp(t,"for ",4)==0){
                char *in_pos=strstr(t," in ");
                if(in_pos){
                    char var[128]; char coll_raw[512];
                    char tmp[256]; strncpy(tmp,t+4,in_pos-(t+4)); tmp[in_pos-(t+4)]='\0'; trim_inplace(tmp); strcpy(var,tmp);
                    strcpy(coll_raw,in_pos+4); trim_inplace(coll_raw);
                    int lim=parse_limit(coll_raw);
                    const char *be=strstr(q,"{% endfor %}");
                    if(be){
                        size_t bl=be-q; char *body=malloc(bl+1); memcpy(body,q,bl); body[bl]='\0'; q=be+12;
                        CAXArray *arr=resolve_array_path(ctx,coll_raw);
                        if(arr && arr->count>0){
                            int max=arr->count; if(lim>0 && lim<max) max=lim;
                            for(int i=0;i<max;i++){
                                void *ptr=arr->items[i]; if(!ptr) continue;
                                CAXObject *sub=cax_object_new();
                                for(int j=0;j<ctx->count;j++){
                                    CAXField *f=&ctx->fields[j];
                                    if(f->type==CAX_TYPE_STRING) cax_object_set_string(sub,f->key,f->string_val);
                                    else cax_object_set_pointer(sub,f->key,f->pointer_val);
                                }
                                cax_object_set_pointer(sub,var,(CAXObject*)ptr);
                                char *rend=cax_template_render(body,sub);
                                cax_object_free(sub);
                                if(rend){ size_t rl=strlen(rend); if(len+rl>=cap){ cap=cap*2+rl+1024; out=realloc(out,cap); }
                                    strcpy(out+len,rend); len+=rl; free(rend);
                                }
                            }
                        }
                        free(body);
                    }
                }
            } else if(strncmp(t,"if ",3)==0){
                char cond[512]; strncpy(cond,t+3,511); cond[511]='\0';
                const char *endif=strstr(q,"{% endif %}"); if(!endif) continue;
                size_t tlen=endif-q; char *blk=malloc(tlen+1); memcpy(blk,q,tlen); blk[tlen]='\0'; q=endif+11;
                char *els=strstr(blk,"{% else %}");
                char *ch=NULL;
                if(is_truthy(ctx,cond)){ if(els) *els='\0'; ch=blk; } else { if(els) ch=els+10; }
                if(ch){ char *rend=cax_template_render(ch,ctx);
                    if(rend){ size_t rl=strlen(rend); if(len+rl>=cap){ cap=cap*2+rl+1024; out=realloc(out,cap); }
                        strcpy(out+len,rend); len+=rl; free(rend);
                    }
                }
                free(blk);
            }
        } else { if(len+1>=cap){ cap*=2; out=realloc(out,cap); } out[len++]=*q++; out[len]='\0'; }
    }
    return out;
}
char *cax_template_render_file(const char *tpl_path, CAXObject *ctx){
    char *content=cax_read_file(tpl_path); if(!content) return NULL;
    if(strncmp(content,"---",3)==0){
        CAXParsedDocument *doc=cax_frontmatter_parse(content); free(content); if(!doc) return NULL;
        const char *parent=cax_object_get_string(doc->metadata,"layout");
        char *child_body=doc->body?cax_strdup(doc->body):cax_strdup("");
        CAXObject *merged=cax_object_new();
        for(int i=0;i<ctx->count;i++){
            if(ctx->fields[i].type==CAX_TYPE_STRING) cax_object_set_string(merged,ctx->fields[i].key,ctx->fields[i].string_val);
            else cax_object_set_pointer(merged,ctx->fields[i].key,ctx->fields[i].pointer_val);
        }
        if(doc->metadata){
            for(int i=0;i<doc->metadata->count;i++){
                if(strcmp(doc->metadata->fields[i].key,"layout")==0) continue;
                if(doc->metadata->fields[i].type==CAX_TYPE_STRING) cax_object_set_string(merged,doc->metadata->fields[i].key,doc->metadata->fields[i].string_val);
                else cax_object_set_pointer(merged,doc->metadata->fields[i].key,doc->metadata->fields[i].pointer_val);
            }
        }
        char *rendered_child=cax_template_render(child_body,merged);
        CAXObject *child_ctx=cax_object_new();
        for(int i=0;i<merged->count;i++){
            if(merged->fields[i].type==CAX_TYPE_STRING) cax_object_set_string(child_ctx,merged->fields[i].key,merged->fields[i].string_val);
            else cax_object_set_pointer(child_ctx,merged->fields[i].key,merged->fields[i].pointer_val);
        }
        cax_object_set_string(child_ctx,"content",rendered_child?rendered_child:"");
        char *result=NULL;
        if(parent && *parent){
            char pp[1024]; snprintf(pp,sizeof(pp),"templates/layouts/%s",parent);
            result=cax_template_render_file(pp,child_ctx);
            if(!result) result=cax_strdup(rendered_child?rendered_child:"");
        } else result=cax_strdup(rendered_child?rendered_child:"");
        free(child_body); if(rendered_child) free(rendered_child);
        cax_object_free(merged); cax_object_free(child_ctx); cax_frontmatter_free(doc);
        return result;
    }
    char *res=cax_template_render(content,ctx); free(content); return res;
}