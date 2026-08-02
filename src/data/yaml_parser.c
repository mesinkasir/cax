#include "yaml_parser.h"
#include "data_manager.h"
#include "../utils/string_utils.h"
#include "../utils/file_utils.h"
#include "../../include/cax.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static char *trim_spaces(char *s){ if(!s) return NULL; while(*s&&isspace((unsigned char)*s)) s++; if(!*s) return s; char *e=s+strlen(s)-1; while(e>s&&isspace((unsigned char)*e)){*e='\0'; e--;} return s; }
static char *strip_quotes(char *s){ if(!s) return s; size_t l=strlen(s); if(l>=2&&((s[0]=='"'&&s[l-1]=='"')||(s[0]=='\''&&s[l-1]=='\''))){ s[l-1]='\0'; return s+1; } return s; }
static int get_indent(const char *line){ int i=0; while(line[i]==' ') i++; return i; }

CAXObject* cax_parse_yaml(const char *yaml_str){
    if(!yaml_str) return NULL;
    CAXObject *root=cax_object_new();
    char *copy=cax_strdup(yaml_str);

    char *lines[2048]; int total=0;
    char *save=NULL; char *l=strtok_r(copy,"\n",&save);
    while(l&&total<2048){ lines[total++]=l; l=strtok_r(NULL,"\n",&save); }

    CAXObject *obj_stack[256]; CAXArray *arr_stack[256]; int ind_stack[256]; int is_arr[256];
    int top=1; obj_stack[0]=root; arr_stack[0]=NULL; ind_stack[0]=-1; is_arr[0]=0;

    for(int idx=0; idx<total; idx++){
        char buf[2048]; strncpy(buf,lines[idx],sizeof(buf)-1); buf[sizeof(buf)-1]='\0';
        char *trim=trim_spaces(buf);
        if(!trim||!strlen(trim)||trim[0]=='#') continue;
        int ind=get_indent(lines[idx]);
        while(top>1 && ind <= ind_stack[top-1]) top--;

        if(trim[0]=='-'){
            char *val=trim_spaces(trim+1); val=strip_quotes(val);
            CAXArray *target=NULL; for(int i=top-1;i>=0;i--) if(is_arr[i]){ target=arr_stack[i]; break; }
            if(!target) continue;
            if(strlen(val)==0){
                CAXObject *item=cax_object_new(); cax_array_add(target,item);
                obj_stack[top]=item; arr_stack[top]=NULL; ind_stack[top]=ind; is_arr[top]=0; top++;
            } else {
                char *colon=strchr(val,':');
                if(colon){
                    *colon='\0'; char *k=trim_spaces(val); char *v=trim_spaces(colon+1);
                    CAXObject *item=cax_object_new(); cax_object_set_string(item,k,strip_quotes(v));
                    cax_array_add(target,item);
                    obj_stack[top]=item; arr_stack[top]=NULL; ind_stack[top]=ind; is_arr[top]=0; top++;
                } else {
                    CAXObject *item=cax_object_new(); cax_object_set_string(item,"value",val); cax_array_add(target,item);
                }
            }
            continue;
        }
        if(is_arr[top-1]) continue;
        char *colon=strchr(trim,':'); if(!colon) continue;
        *colon='\0'; char *key=trim_spaces(trim); char *val=trim_spaces(colon+1);
        if(!strlen(key)) continue;
        if(strlen(val)==0){
            int next_is_arr=0;
            for(int j=idx+1;j<total;j++){ char b2[2048]; strncpy(b2,lines[j],sizeof(b2)-1); char *t2=trim_spaces(b2); if(!t2||!strlen(t2)||t2[0]=='#') continue; int ind2=get_indent(lines[j]); if(ind2<=ind) break; if(t2[0]=='-') next_is_arr=1; break; }
            if(next_is_arr){ CAXArray *arr=cax_array_new(); cax_object_set_pointer(obj_stack[top-1],key,arr); obj_stack[top]=NULL; arr_stack[top]=arr; ind_stack[top]=ind; is_arr[top]=1; top++; }
            else { CAXObject *o=cax_object_new(); cax_object_set_pointer(obj_stack[top-1],key,o); obj_stack[top]=o; arr_stack[top]=NULL; ind_stack[top]=ind; is_arr[top]=0; top++; }
        } else if(val[0]=='['){
            CAXArray *arr=cax_array_new(); char *p=val+1;
            while(*p){ while(*p==' '||*p=='\t'||*p==','||*p=='"'||*p=='\''||*p=='[') p++; if(*p==']'||!*p) break; char *s=p; while(*p&&*p!=','&&*p!=']'&&*p!='"'&&*p!='\'') p++; int len=p-s; if(len>0){ char tmp[256]; if(len>255) len=255; memcpy(tmp,s,len); tmp[len]='\0'; char *t=trim_spaces(tmp); t=strip_quotes(t); if(strlen(t)){ CAXObject *it=cax_object_new(); cax_object_set_string(it,"value",t); cax_array_add(arr,it); } } }
            cax_object_set_pointer(obj_stack[top-1],key,arr);
        } else {
            cax_object_set_string(obj_stack[top-1],key,strip_quotes(val));
        }
    }
    free(copy); return root;
}

void cax_load_yaml_file(const char *filepath, CAXObject *global_ctx){
    char *content=cax_read_file(filepath); if(!content) return;
    CAXObject *parsed=cax_parse_yaml(content); free(content); if(!parsed) return;
    const char *filename=strrchr(filepath,'/'); if(!filename) filename=strrchr(filepath,'\\'); filename=filename?filename+1:filepath;
    char name[256]={0}; strncpy(name,filename,sizeof(name)-1); char *ext=strrchr(name,'.'); if(ext) *ext='\0';
    CAXObject *existing=(CAXObject*)cax_object_get_pointer(global_ctx,name);
    if(existing){
        for(int i=0;i<parsed->count;i++){
            if(parsed->fields[i].type==CAX_TYPE_STRING) cax_object_set_string(existing,parsed->fields[i].key,parsed->fields[i].string_val);
            else cax_object_set_pointer(existing,parsed->fields[i].key,parsed->fields[i].pointer_val);
        }
        cax_object_free(parsed);
    } else {
        cax_object_set_pointer(global_ctx,name,parsed);
    }
    printf(" [DATA] Loaded %s -> {{ %s.* }}\n", filepath, name);
}