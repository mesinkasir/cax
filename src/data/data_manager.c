#include "data_manager.h"
#include "yaml_parser.h"
#include "json_parser.h"
#include "../utils/file_utils.h"
#include "../utils/string_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

CAXArray* cax_array_new(void){
    CAXArray *a=malloc(sizeof(CAXArray));
    if(!a) return NULL;
    a->count=0; a->capacity=8;
    a->items=malloc(a->capacity*sizeof(void*));
    if(!a->items){ free(a); return NULL; }
    return a;
}
void cax_array_add(CAXArray *arr, void *item){
    if(!arr||!item) return;
    if(arr->count>=arr->capacity){
        arr->capacity*=2;
        void **ni=realloc(arr->items, arr->capacity*sizeof(void*));
        if(!ni) return;
        arr->items=ni;
    }
    arr->items[arr->count++]=item;
}
void cax_array_free(CAXArray *arr){ if(!arr) return; free(arr->items); free(arr); }

CAXObject* cax_object_new(void){
    CAXObject *o=malloc(sizeof(CAXObject));
    if(!o) return NULL;
    o->count=0; o->capacity=16;
    o->fields=calloc(o->capacity,sizeof(CAXField));
    if(!o->fields){ free(o); return NULL; }
    return o;
}
void cax_object_free(CAXObject *obj){
    if(!obj) return;
    for(int i=0;i<obj->count;i++){
        free(obj->fields[i].key);
        if(obj->fields[i].type==CAX_TYPE_STRING) free(obj->fields[i].string_val);
    }
    free(obj->fields); free(obj);
}
void cax_object_set_string(CAXObject *obj, const char *key, const char *val){
    if(!obj||!key) return;
    for(int i=0;i<obj->count;i++) if(strcmp(obj->fields[i].key,key)==0){
        if(obj->fields[i].type==CAX_TYPE_STRING) free(obj->fields[i].string_val);
        obj->fields[i].type=CAX_TYPE_STRING;
        obj->fields[i].string_val=cax_strdup(val?val:"");
        return;
    }
    if(obj->count>=obj->capacity){
        obj->capacity*=2;
        CAXField *nf=realloc(obj->fields,obj->capacity*sizeof(CAXField));
        if(!nf) return;
        obj->fields=nf;
    }
    obj->fields[obj->count].key=cax_strdup(key);
    obj->fields[obj->count].type=CAX_TYPE_STRING;
    obj->fields[obj->count].string_val=cax_strdup(val?val:"");
    obj->count++;
}
void cax_object_set_pointer(CAXObject *obj, const char *key, void *val){
    if(!obj||!key) return;
    for(int i=0;i<obj->count;i++) if(strcmp(obj->fields[i].key,key)==0){
        if(obj->fields[i].type==CAX_TYPE_STRING) free(obj->fields[i].string_val);
        obj->fields[i].type=CAX_TYPE_POINTER;
        obj->fields[i].pointer_val=val;
        return;
    }
    if(obj->count>=obj->capacity){
        obj->capacity*=2;
        CAXField *nf=realloc(obj->fields,obj->capacity*sizeof(CAXField));
        if(!nf) return;
        obj->fields=nf;
    }
    obj->fields[obj->count].key=cax_strdup(key);
    obj->fields[obj->count].type=CAX_TYPE_POINTER;
    obj->fields[obj->count].pointer_val=val;
    obj->count++;
}
const char* cax_object_get_string(CAXObject *obj, const char *key){
    if(!obj||!key) return NULL;
    for(int i=0;i<obj->count;i++) if(strcmp(obj->fields[i].key,key)==0 && obj->fields[i].type==CAX_TYPE_STRING) return obj->fields[i].string_val;
    return NULL;
}
void* cax_object_get_pointer(CAXObject *obj, const char *key){
    if(!obj||!key) return NULL;
    for(int i=0;i<obj->count;i++) if(strcmp(obj->fields[i].key,key)==0 && obj->fields[i].type==CAX_TYPE_POINTER) return obj->fields[i].pointer_val;
    return NULL;
}
void cax_object_set_array(CAXObject *obj, const char *key, CAXArray *arr){ cax_object_set_pointer(obj,key,arr); }
CAXArray* cax_object_get_array(CAXObject *obj, const char *key){ return (CAXArray*)cax_object_get_pointer(obj,key); }

void cax_object_set_int(CAXObject *obj, const char *key, int val){
    if(!obj||!key) return;
    for(int i=0;i<obj->count;i++) if(strcmp(obj->fields[i].key,key)==0){
        if(obj->fields[i].type==CAX_TYPE_STRING) free(obj->fields[i].string_val);
        obj->fields[i].type=CAX_TYPE_INT; obj->fields[i].int_val=val; return;
    }
    if(obj->count>=obj->capacity){ obj->capacity*=2; CAXField *nf=realloc(obj->fields,obj->capacity*sizeof(CAXField)); if(!nf) return; obj->fields=nf; }
    obj->fields[obj->count].key=cax_strdup(key);
    obj->fields[obj->count].type=CAX_TYPE_INT;
    obj->fields[obj->count].int_val=val;
    obj->count++;
}
int cax_object_get_int(CAXObject *obj, const char *key){
    if(!obj||!key) return 0;
    for(int i=0;i<obj->count;i++) if(strcmp(obj->fields[i].key,key)==0 && obj->fields[i].type==CAX_TYPE_INT) return obj->fields[i].int_val;
    return 0;
}
void cax_array_push(CAXArray *arr, void *item){ cax_array_add(arr,item); }

void cax_load_data_file(const char *filepath, CAXObject *global_ctx){
    if(!filepath||!global_ctx) return;
    if(cax_is_directory(filepath)){ cax_load_all_data(filepath,global_ctx); return; }
    const char *ext=strrchr(filepath,'.');
    if(!ext) return;
    if(strcmp(ext,".json")==0) cax_load_json_file(filepath,global_ctx);
    else if(strcmp(ext,".yaml")==0||strcmp(ext,".yml")==0) cax_load_yaml_file(filepath,global_ctx);
}

void cax_load_all_data(const char *data_dir, CAXObject *global_ctx){
    DIR *dir=opendir(data_dir);
    if(!dir) return;
    struct dirent *e;
    while((e=readdir(dir))!=NULL){
        if(e->d_name[0]=='.') continue;
        char path[1024]; snprintf(path,sizeof(path),"%s/%s",data_dir,e->d_name);
        if(cax_is_directory(path)){ cax_load_all_data(path,global_ctx); continue; }
        const char *ext=strrchr(e->d_name,'.');
        if(!ext) continue;
        if(strcmp(ext,".json")!=0 && strcmp(ext,".yaml")!=0 && strcmp(ext,".yml")!=0) continue;
        if(strcmp(ext,".json")==0) cax_load_json_file(path,global_ctx);
        else cax_load_yaml_file(path,global_ctx);
    }
    closedir(dir);
}