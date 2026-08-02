#ifndef SITEMAP_H
#define SITEMAP_H
#include "../../include/cax.h"
void cax_sitemap_generate(CAXObject *global_data, const char *output_dir);
void cax_sitemap_generate_full(CAXArray *all_pages, CAXObject *global_data, const char *output_dir, CAXObject *controllers);
#endif