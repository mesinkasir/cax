#include "pagination.h"
#include "../utils/file_utils.h"
#include "../template/template_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void copy_all(CAXObject *s, CAXObject *d){
    if(!s||!d) return;
    for(int i=0;i<s->count;i++){
        if(s->fields[i].type==CAX_TYPE_STRING){
            if(strcmp(s->fields[i].key,"pagination")==0 && atoi(s->fields[i].string_val)>0) continue;
            cax_object_set_string(d,s->fields[i].key,s->fields[i].string_val);
        } else cax_object_set_pointer(d,s->fields[i].key,s->fields[i].pointer_val);
    }
}
void cax_pagination_process(CAXArray *coll_array, int per_page, const char *out_dir, const char *tpl, CAXObject *global_data, CAXObject *pagination_controllers, const char *coll_name){
    if(!coll_array||!out_dir) return;
    if(per_page<=0) per_page=5;
    int total=coll_array->count;
    int pages=(total+per_page-1)/per_page; if(pages==0) pages=1;
    CAXObject *ctrl=NULL;
    if(pagination_controllers && coll_name) ctrl=cax_object_get_pointer(pagination_controllers,coll_name);
    for(int p=0;p<pages;p++){
        CAXObject *ctx=cax_object_new();
        if(global_data) copy_all(global_data,ctx);
        if(ctrl) copy_all(ctrl,ctx);
        CAXArray *slice=cax_array_new();
        int s=p*per_page, e=s+per_page; if(e>total) e=total;
        for(int i=s;i<e;i++) cax_array_add(slice,coll_array->items[i]);
        CAXObject *paginator=cax_object_new();
        cax_object_set_pointer(paginator,"items",slice);
        cax_object_set_pointer(paginator,"posts",slice);
        if(p==1){
            char u[256]; snprintf(u,sizeof(u),"/%s/",coll_name);
            cax_object_set_string(paginator,"prev_url",u);
        } else if(p>1){
            char u[256]; snprintf(u,sizeof(u),"/%s/page/%d/",coll_name,p);
            cax_object_set_string(paginator,"prev_url",u);
        }
        if(p < pages-1){
            char u[256]; snprintf(u,sizeof(u),"/%s/page/%d/",coll_name,p+2);
            cax_object_set_string(paginator,"next_url",u);
        }
        char b1[16], b2[16]; snprintf(b1,sizeof(b1),"%d",p+1); snprintf(b2,sizeof(b2),"%d",pages);
        cax_object_set_string(paginator,"page",b1);
        cax_object_set_string(paginator,"total_pages",b2);
        cax_object_set_pointer(ctx,"pagination",paginator);
        cax_object_set_pointer(ctx,"paginator",paginator);
        cax_object_set_pointer(ctx,"posts",slice);
        char *html=cax_template_render_file(tpl,ctx);
        char out[1024];
        if(p==0) snprintf(out,sizeof(out),"%s/index.html",out_dir);
        else { char d[1024]; snprintf(d,sizeof(d),"%s/page/%d",out_dir,p+1); cax_create_directory(d); snprintf(out,sizeof(out),"%s/index.html",d); }
        cax_write_file(out, html?html:"");
        free(html);
        cax_object_free(paginator);
        free(slice->items); free(slice);
        cax_object_free(ctx);
    }
}