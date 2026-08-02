#include "path_utils.h"
#include <stdio.h>
#include <string.h>

void cax_path_join(char *dest, size_t size, const char *p1, const char *p2) {
    if (!p1 || !p2) return;
    snprintf(dest, size, "%s/%s", p1, p2);
}

void cax_path_get_dirname(const char *path, char *dest, size_t size) {
    if (!path) return;
    const char *last_slash = strrchr(path, '/');
    if (!last_slash) last_slash = strrchr(path, '\\');
    
    if (last_slash) {
        size_t len = last_slash - path;
        if (len >= size) len = size - 1;
        memcpy(dest, path, len);
        dest[len] = '\0';
    } else {
        strncpy(dest, ".", size);
    }
}