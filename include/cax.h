#ifndef CAX_H
#define CAX_H

#include <stddef.h>

typedef enum {
    CAX_TYPE_STRING,
    CAX_TYPE_INT,
    CAX_TYPE_POINTER,
    CAX_TYPE_ARRAY
} CAXFieldType;

typedef struct {
    char *key;
    CAXFieldType type;
    union {
        char *string_val;
        int int_val;
        void *pointer_val;
    };
} CAXField;

typedef struct {
    CAXField *fields;
    int count;
    int capacity;
} CAXObject;

typedef struct {
    void **items;
    int count;
    int capacity;
} CAXArray;

CAXObject* cax_object_new(void);
void cax_object_free(CAXObject *obj);
void cax_object_set_string(CAXObject *obj, const char *key, const char *val);
void cax_object_set_int(CAXObject *obj, const char *key, int val);
void cax_object_set_pointer(CAXObject *obj, const char *key, void *val);
void cax_object_set_array(CAXObject *obj, const char *key, CAXArray *arr);
const char* cax_object_get_string(CAXObject *obj, const char *key);
int cax_object_get_int(CAXObject *obj, const char *key);
void* cax_object_get_pointer(CAXObject *obj, const char *key);
CAXArray* cax_object_get_array(CAXObject *obj, const char *key);

CAXArray* cax_array_new(void);
void cax_array_free(CAXArray *arr);
void cax_array_push(CAXArray *arr, void *item);
void cax_array_add(CAXArray *arr, void *item);

#endif