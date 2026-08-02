#ifndef PATH_UTILS_H
#define PATH_UTILS_H

#include <stddef.h>

void cax_path_join(char *dest, size_t size, const char *p1, const char *p2);
void cax_path_get_dirname(const char *path, char *dest, size_t size);

#endif