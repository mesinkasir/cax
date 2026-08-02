#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H
#include "../../include/cax.h"

CAXObject* cax_object_new(void);
void cax_object_free(CAXObject *obj);
void cax_object_set_string(CAXObject *obj, const char *key, const char *val);
void cax_object_set_pointer(CAXObject *obj, const char *key, void *val);
const char* cax_object_get_string(CAXObject *obj, const char *key);
void* cax_object_get_pointer(CAXObject *obj, const char *key);

CAXArray* cax_array_new(void);
void cax_array_add(CAXArray *arr, void *item);
void cax_array_free(CAXArray *arr);

void cax_object_set_array(CAXObject *obj, const char *key, CAXArray *arr);
CAXArray* cax_object_get_array(CAXObject *obj, const char *key);

void cax_load_data_file(const char *filepath, CAXObject *global_ctx);
void cax_load_json_file(const char *filepath, CAXObject *global_ctx);
void cax_load_yaml_file(const char *filepath, CAXObject *global_ctx);
void cax_load_all_data(const char *data_dir, CAXObject *global_ctx); 

#endif