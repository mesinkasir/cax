#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <stdio.h>

char* cax_read_file(const char *filepath);
int cax_write_file(const char *filepath, const char *content);
int cax_is_directory(const char *path);
int cax_create_directory(const char *path);
void cax_copy_file(const char *src, const char *dest);
void cax_copy_directory(const char *src_dir, const char *dest_dir);

#endif