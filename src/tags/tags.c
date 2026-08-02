#include "tags.h"
#include "../data/data_manager.h"
#include "../template/template_engine.h"
#include "../utils/file_utils.h"
#include "../utils/string_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void add_unique_tag(CAXArray *unique_tags, const char *raw_tag) {
    if (!raw_tag) return;
    char tag[256]; strncpy(tag, raw_tag, sizeof(tag)-1); tag[255]='\0';
    char *t=tag; while(*t==' '||*t=='-'||*t=='"'||*t=='\''||*t=='['||*t==']') t++;
    size_t len=strlen(t); while(len>0 && (t[len-1]==' '||t[len-1]=='"'||t[len-1]=='\''||t[len-1]==']')) t[--len]='\0';
    if(!*t) return;
    for(int k=0;k<unique_tags->count;k++){
        CAXObject *obj=(CAXObject*)unique_tags->items[k];
        const char *ex=cax_object_get_string(obj,"name");
        if(ex && strcmp(ex,t)==0) return;
    }
    if(unique_tags->count>=unique_tags->capacity){
        int cap=unique_tags->capacity==0?16:unique_tags->capacity*2;
        void **n=realloc(unique_tags->items,cap*sizeof(void*));
        if(!n) return; unique_tags->items=n; unique_tags->capacity=cap;
    }
    CAXObject *o=cax_object_new();
    cax_object_set_string(o,"name",t);
    cax_object_set_string(o,"value",t);
    unique_tags->items[unique_tags->count++]=o;
}
static void copy_ctx(CAXObject *d, CAXObject *s){
    if(!d||!s) return;
    for(int i=0;i<s->count;i++){
        if(s->fields[i].type==CAX_TYPE_STRING) cax_object_set_string(d,s->fields[i].key,s->fields[i].string_val);
        else cax_object_set_pointer(d,s->fields[i].key,s->fields[i].pointer_val);
    }
}
static const char* get_val(void *p){
    if(!p) return NULL;
    CAXObject *o=(CAXObject*)p;
    const char *v=cax_object_get_string(o,"value");
    if(v) return v;
    v=cax_object_get_string(o,"name");
    if(v) return v;
    return (const char*)p;
}
void cax_tags_generate(CAXArray *master_posts, const char *output_dir, CAXObject *global_data){
    if(!master_posts||!output_dir) return;
    CAXArray *uniq=calloc(1,sizeof(CAXArray));
    uniq->capacity=16; uniq->items=calloc(uniq->capacity,sizeof(void*));
    for(int i=0;i<master_posts->count;i++){
        CAXObject *post=(CAXObject*)master_posts->items[i];
        CAXArray *arr=(CAXArray*)cax_object_get_pointer(post,"tags");
        if(arr && arr->count>0){
            for(int j=0;j<arr->count;j++){ const char *v=get_val(arr->items[j]); if(v) add_unique_tag(uniq,v); }
        } else {
            const char *s=cax_object_get_string(post,"tags");
            if(s){ char *c=cax_strdup(s); char *sv; char *tok=strtok_r(c,",[] ",&sv); while(tok){ add_unique_tag(uniq,tok); tok=strtok_r(NULL,",[] ",&sv);} free(c); }
        }
    }
    char tags_dir[1024]; snprintf(tags_dir,sizeof(tags_dir),"%s/tags",output_dir);
    cax_create_directory(tags_dir);
    CAXObject *ctx=cax_object_new(); copy_ctx(ctx,global_data);
    cax_object_set_pointer(ctx,"tags",uniq);
    // FIX URL UNTUK /tags/
    cax_object_set_string(ctx,"url","/tags/");
    cax_object_set_string(ctx,"permalink","/tags/");
    cax_object_set_string(ctx,"title","Tags");
    char *html=cax_template_render_file("templates/layouts/tags.cax",ctx);
    if(!html) html=cax_template_render_file("templates/layouts/default.cax",ctx);
    char path[2048]; snprintf(path,sizeof(path),"%s/index.html",tags_dir);
    cax_write_file(path,html?html:""); free(html); cax_object_free(ctx);

    for(int i=0;i<uniq->count;i++){
        CAXObject *to=(CAXObject*)uniq->items[i];
        const char *tname=cax_object_get_string(to,"name"); if(!tname) continue;
        CAXArray *tposts=calloc(1,sizeof(CAXArray)); tposts->capacity=8; tposts->items=calloc(tposts->capacity,sizeof(void*));
        for(int j=0;j<master_posts->count;j++){
            CAXObject *post=(CAXObject*)master_posts->items[j]; int has=0;
            CAXArray *arr=(CAXArray*)cax_object_get_pointer(post,"tags");
            if(arr){ for(int k=0;k<arr->count;k++){ const char *v=get_val(arr->items[k]); if(v && strcmp(v,tname)==0){ has=1; break; } } }
            else { const char *s=cax_object_get_string(post,"tags"); if(s && strstr(s,tname)) has=1; }
            if(has){ if(tposts->count>=tposts->capacity){ tposts->capacity*=2; tposts->items=realloc(tposts->items,tposts->capacity*sizeof(void*)); } tposts->items[tposts->count++]=post; }
        }
        char dir[2048]; snprintf(dir,sizeof(dir),"%s/%s",tags_dir,tname); cax_create_directory(dir);
        CAXObject *c=cax_object_new(); copy_ctx(c,global_data);
        cax_object_set_string(c,"tag_name",tname); cax_object_set_string(c,"title",tname);
        // FIX URL UNTUK /tags/jekyll/ dll
        char url_buf[1024]; snprintf(url_buf,sizeof(url_buf),"/tags/%s/",tname);
        cax_object_set_string(c,"url",url_buf);
        cax_object_set_string(c,"permalink",url_buf);
        cax_object_set_string(c,"slug",tname);
        cax_object_set_pointer(c,"posts",tposts); cax_object_set_pointer(c,"items",tposts);
        char *h=cax_template_render_file("templates/layouts/tag.cax",c);
        if(!h) h=cax_template_render_file("templates/layouts/default.cax",c);
        char fp[4096]; snprintf(fp,sizeof(fp),"%s/index.html",dir);
        cax_write_file(fp,h?h:""); free(h); cax_object_free(c); free(tposts->items); free(tposts);
    }
    for(int i=0;i<uniq->count;i++) cax_object_free((CAXObject*)uniq->items[i]);
    free(uniq->items); free(uniq);
}