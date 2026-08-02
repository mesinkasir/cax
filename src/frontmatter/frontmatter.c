#include "frontmatter.h"
#include "../utils/string_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static char *trim_spaces(char *str){ if(!str) return NULL; while(*str && isspace((unsigned char)*str)) str++; if(*str=='\0') return str; char *e=str+strlen(str)-1; while(e>str && isspace((unsigned char)*e)){*e='\0'; e--;} return str; }
static char *strip_quotes(char *s){ if(!s) return s; size_t l=strlen(s); if(l>=2 && ((s[0]=='"'&&s[l-1]=='"')||(s[0]=='\''&&s[l-1]=='\''))){ s[l-1]='\0'; return s+1; } return s; }
static int get_indent(const char *line){ int i=0; while(line[i]==' ') i++; return i; }

CAXParsedDocument *cax_frontmatter_parse(const char *raw_content) {
    if (!raw_content) return NULL;
    CAXParsedDocument *doc = malloc(sizeof(CAXParsedDocument));
    doc->metadata = cax_object_new();
    doc->body = NULL;

    const char *fs = strstr(raw_content, "---");
    if (!fs) { doc->body=cax_strdup(raw_content); return doc; }
    const char *fe = strstr(fs+3, "---");
    if (!fe) { doc->body=cax_strdup(raw_content); return doc; }

    size_t fl = fe - (fs+3);
    char *fb = malloc(fl+1); memcpy(fb, fs+3, fl); fb[fl]='\0';
    doc->body = cax_strdup(fe+3);

    char *lines[2048]; int total=0; char *save=NULL; char *l=strtok_r(fb, "\n", &save);
    while(l && total<2048){ lines[total++]=l; l=strtok_r(NULL, "\n", &save); }

    CAXObject *obj_stack[256]; CAXArray *arr_stack[256]; int ind_stack[256]; int is_arr[256];
    int top=1; obj_stack[0]=doc->metadata; arr_stack[0]=NULL; ind_stack[0]=-1; is_arr[0]=0;

    for(int idx=0; idx<total; idx++){
        char *line=lines[idx];
        char buf[2048]; strncpy(buf,line,sizeof(buf)-1); buf[sizeof(buf)-1]='\0';
        char *trim=trim_spaces(buf);
        if(!trim ||!strlen(trim) || trim[0]=='#') continue;
        int ind=get_indent(line);
        while(top>1 && ind <= ind_stack[top-1]) top--;

        // LIST ITEM
        if(trim[0]=='-'){
            char *val=trim_spaces(trim+1);
            CAXArray *target=NULL;
            for(int i=top-1;i>=0;i--) if(is_arr[i]){ target=arr_stack[i]; break; }
            if(!target) continue;

            if(strlen(val)==0){
                // - \n title: foo (list of objects)
                CAXObject *item=cax_object_new();
                cax_array_add(target, item);
                obj_stack[top]=item; arr_stack[top]=NULL; ind_stack[top]=ind; is_arr[top]=0; top++;
            } else {
                char *colon=strchr(val, ':');
                if(colon){
                    // - title: 0.4s Builds
                    *colon='\0'; char *k=trim_spaces(val); char *v=trim_spaces(colon+1);
                    CAXObject *item=cax_object_new();
                    cax_object_set_string(item, k, strip_quotes(v));
                    cax_array_add(target, item);
                    // push item biar baris selanjutnya text:... masuk ke item ini
                    obj_stack[top]=item; arr_stack[top]=NULL; ind_stack[top]=ind; is_arr[top]=0; top++;
                } else {
                    CAXObject *item=cax_object_new();
                    cax_object_set_string(item, "value", strip_quotes(val));
                    cax_array_add(target, item);
                }
            }
            continue;
        }

        if(is_arr[top-1]) continue; // lagi di dalam array tapi bukan list item

        char *colon=strchr(trim, ':');
        if(!colon) continue;
        *colon='\0'; char *key=trim_spaces(trim); char *val=trim_spaces(colon+1);

        if(strlen(val)==0){
            int next_is_arr=0;
            for(int j=idx+1;j<total;j++){
                char b2[2048]; strncpy(b2,lines[j],sizeof(b2)-1); b2[sizeof(b2)-1]='\0';
                char *t2=trim_spaces(b2); if(!t2||!strlen(t2)||t2[0]=='#') continue;
                int ind2=get_indent(lines[j]); if(ind2 <= ind) break;
                if(t2[0]=='-') next_is_arr=1; break;
            }
            if(next_is_arr){
                CAXArray *arr=cax_array_new();
                cax_object_set_pointer(obj_stack[top-1], key, arr);
                obj_stack[top]=NULL; arr_stack[top]=arr; ind_stack[top]=ind; is_arr[top]=1; top++;
            } else {
                CAXObject *o=cax_object_new();
                cax_object_set_pointer(obj_stack[top-1], key, o);
                obj_stack[top]=o; arr_stack[top]=NULL; ind_stack[top]=ind; is_arr[top]=0; top++;
            }
        } else if(val[0]=='['){
            CAXArray *arr=cax_array_new(); char *p=val+1;
            while(*p){ while(*p==' '||*p=='\t'||*p==','||*p=='"'||*p=='\''||*p=='[') p++; if(*p==']'||!*p) break; char *s=p; while(*p && *p!=','&&*p!=']'&&*p!='"'&&*p!='\'') p++; int len=p-s; if(len>0){ char tmp[256]; if(len>255) len=255; memcpy(tmp,s,len); tmp[len]='\0'; char *t=trim_spaces(tmp); t=strip_quotes(t); if(strlen(t)){ CAXObject *it=cax_object_new(); cax_object_set_string(it,"value",t); cax_array_add(arr,it); } } }
            cax_object_set_pointer(obj_stack[top-1], key, arr);
        } else {
            cax_object_set_string(obj_stack[top-1], key, strip_quotes(val));
        }
    }
    free(fb);
    return doc;
}
void cax_frontmatter_free(CAXParsedDocument *doc){ if(!doc) return; if(doc->metadata) cax_object_free(doc->metadata); if(doc->body) free(doc->body); free(doc); }