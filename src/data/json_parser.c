#include "json_parser.h"
#include "../utils/file_utils.h"
#include "../utils/string_utils.h"
#include "../data/data_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* trim_json_string(char *str) {
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') str++;
    if (*str == '\0') return str;
    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }
    return str;
}

static char* parse_string(char **p) {
    if (**p != '"') return NULL;
    (*p)++;
    char *start = *p;
    while (**p && **p != '"') (*p)++;
    if (**p == '"') {
        size_t len = *p - start;
        char *result = malloc(len + 1);
        if (!result) return NULL;
        memcpy(result, start, len);
        result[len] = '\0';
        (*p)++;
        return result;
    }
    return NULL;
}

static CAXObject* parse_object(char **p);
static CAXArray* parse_array(char **p);

static void parse_value(char **p, CAXObject *obj, const char *key) {
    while (**p == ' ' || **p == '\t' || **p == '\n' || **p == '\r') (*p)++;
    
    if (**p == '{') {
        CAXObject *nested = parse_object(p);
        if (nested) {
            cax_object_set_pointer(obj, key, nested);
        }
    } else if (**p == '[') {
        CAXArray *arr = parse_array(p);
        if (arr) {
            cax_object_set_pointer(obj, key, arr);
        }
    } else if (**p == '"') {
        char *str = parse_string(p);
        if (str) {
            cax_object_set_string(obj, key, str);
            free(str);
        }
    } else if (**p == 't' || **p == 'f') {
        if (strncmp(*p, "true", 4) == 0) {
            cax_object_set_string(obj, key, "true");
            *p += 4;
        } else if (strncmp(*p, "false", 5) == 0) {
            cax_object_set_string(obj, key, "false");
            *p += 5;
        }
    } else if ((**p >= '0' && **p <= '9') || **p == '-') {
        char *start = *p;
        if (**p == '-') (*p)++;
        while ((**p >= '0' && **p <= '9') || **p == '.') (*p)++;
        size_t len = *p - start;
        char *num = malloc(len + 1);
        if (num) {
            memcpy(num, start, len);
            num[len] = '\0';
            cax_object_set_string(obj, key, num);
            free(num);
        }
    }
}

static CAXArray* parse_array(char **p) {
    if (**p != '[') return NULL;
    (*p)++;
    
    CAXArray *arr = cax_array_new();
    if (!arr) return NULL;
    
    while (**p && **p != ']') {
        while (**p == ' ' || **p == '\t' || **p == '\n' || **p == '\r') (*p)++;
        if (**p == ']') break;

        if (**p == '{') {
            CAXObject *nested = parse_object(p);
            if (nested) {
                cax_array_add(arr, nested);
            }
        } else if (**p == '"') {
            char *str = parse_string(p);
            if (str) {
                CAXObject *item = cax_object_new();
                if (item) {
                    cax_object_set_string(item, "value", str);
                    cax_array_add(arr, item);
                }
                free(str);
            }
        } else if (**p == '[') {
            CAXArray *nested_arr = parse_array(p);
            if (nested_arr) {
                CAXObject *item = cax_object_new();
                if (item) {
                    cax_object_set_pointer(item, "value", nested_arr);
                    cax_array_add(arr, item);
                }
            }
        } else {
            char *start = *p;
            while (**p && **p != ',' && **p != ']') (*p)++;
            size_t len = *p - start;
            char *val = malloc(len + 1);
            if (val) {
                memcpy(val, start, len);
                val[len] = '\0';
                CAXObject *item = cax_object_new();
                if (item) {
                    cax_object_set_string(item, "value", trim_json_string(val));
                    cax_array_add(arr, item);
                }
                free(val);
            }
        }
        
        while (**p == ' ' || **p == '\t' || **p == '\n' || **p == '\r') (*p)++;
        if (**p == ',') {
            (*p)++;
            while (**p == ' ' || **p == '\t' || **p == '\n' || **p == '\r') (*p)++;
        }
    }
    if (**p == ']') (*p)++;
    return arr;
}

static CAXObject* parse_object(char **p) {
    if (**p != '{') return NULL;
    (*p)++;
    
    CAXObject *obj = cax_object_new();
    if (!obj) return NULL;
    
    while (**p && **p != '}') {
        while (**p == ' ' || **p == '\t' || **p == '\n' || **p == '\r') (*p)++;
        if (**p == '}') break;

        char *key = parse_string(p);
        if (!key) break;
        
        while (**p == ' ' || **p == '\t' || **p == '\n' || **p == '\r') (*p)++;
        if (**p == ':') {
            (*p)++;
            parse_value(p, obj, key);
        }
        free(key);
        
        while (**p == ' ' || **p == '\t' || **p == '\n' || **p == '\r') (*p)++;
        if (**p == ',') {
            (*p)++;
            while (**p == ' ' || **p == '\t' || **p == '\n' || **p == '\r') (*p)++;
        }
    }
    if (**p == '}') (*p)++;
    return obj;
}

void cax_load_json_file(const char *filepath, CAXObject *global_ctx) {
    if (!filepath || !global_ctx) return;
    
    char *content = cax_read_file(filepath);
    if (!content) return;
    
    const char *filename = strrchr(filepath, '/');
    if (!filename) filename = strrchr(filepath, '\\');
    filename = filename ? filename + 1 : filepath;
    
    char data_name[256] = {0};
    strncpy(data_name, filename, sizeof(data_name) - 1);
    char *ext = strrchr(data_name, '.');
    if (ext) *ext = '\0';
    
    char *p = content;
    CAXObject *data_obj = parse_object(&p);
    free(content);
    
    if (data_obj) {
        cax_object_set_pointer(global_ctx, data_name, data_obj);
    }
}