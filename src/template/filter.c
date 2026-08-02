#include "filter.h"
#include "../utils/string_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

CAXArray* cax_filter_apply(CAXArray *input_array, const char *filter_expr) {
    if (!input_array || !filter_expr) return input_array;
    
    char expr[128];
    strncpy(expr, filter_expr, sizeof(expr) - 1);
    char *trimmed = cax_strtrim(expr);
    
    CAXArray *filtered = malloc(sizeof(CAXArray));
    filtered->count = 0;
    filtered->capacity = input_array->capacity;
    filtered->items = malloc(filtered->capacity * sizeof(CAXObject*));
    
    if (strncmp(trimmed, "limit:", 6) == 0) {
        int limit_val = atoi(trimmed + 6);
        if (limit_val <= 0) limit_val = input_array->count;
        
        for (int i = 0; i < input_array->count && i < limit_val; i++) {
            filtered->items[filtered->count++] = input_array->items[i];
        }
    } else if (strncmp(trimmed, "by:", 3) == 0) {
        char *tag_name = cax_strtrim(trimmed + 3);
        for (int i = 0; i < input_array->count; i++) {
            const char *tags = cax_object_get_string(input_array->items[i], "tags");
            if (tags && strstr(tags, tag_name)) {
                filtered->items[filtered->count++] = input_array->items[i];
            }
        }
    } else {
        for (int i = 0; i < input_array->count; i++) {
            filtered->items[filtered->count++] = input_array->items[i];
        }
    }
    
    return filtered;
}