#include "evaluator.h"
#include "../utils/string_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* resolve_variable(const char *var_expr, CAXObject *context) {
    char expr[128];
    strncpy(expr, var_expr, sizeof(expr) - 1);
    char *trimmed = cax_strtrim(expr);
    
    char *or_pos = strstr(trimmed, " or ");
    if (or_pos) {
        *or_pos = '\0';
        char *first_var = cax_strtrim(trimmed);
        char *second_var = cax_strtrim(or_pos + 4);
        
        const char *val = cax_object_get_string(context, first_var);
        if (val && strlen(val) > 0) return cax_strdup(val);
        
        val = cax_object_get_string(context, second_var);
        if (val && strlen(val) > 0) return cax_strdup(val);
        return cax_strdup("");
    }
    
    const char *val = cax_object_get_string(context, trimmed);
    if (val) return cax_strdup(val);
    return cax_strdup("");
}

char* cax_template_eval(ASTNode *ast, CAXObject *context) {
    if (!ast) return cax_strdup("");
    
    size_t len = 8192;
    char *output = malloc(len);
    if (!output) return NULL;
    output[0] = '\0';
    
    size_t remaining = len;
    char *out_ptr = output;
    
    ASTNode *curr = ast;
    while (curr) {
        if (curr->type == NODE_TEXT) {
            if (curr->value) {
                size_t v_len = strlen(curr->value);
                if (v_len < remaining) {
                    strcpy(out_ptr, curr->value);
                    out_ptr += v_len;
                    remaining -= v_len;
                }
            }
        } else if (curr->type == NODE_VAR) {
            char *resolved = resolve_variable(curr->value, context);
            if (resolved) {
                size_t r_len = strlen(resolved);
                if (r_len < remaining) {
                    strcpy(out_ptr, resolved);
                    out_ptr += r_len;
                    remaining -= r_len;
                }
                free(resolved);
            }
        }
        curr = curr->next;
    }
    
    return output;
}