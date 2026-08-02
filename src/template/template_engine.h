#ifndef TEMPLATE_ENGINE_H
#define TEMPLATE_ENGINE_H
#include "../data/data_manager.h"

char *cax_template_render(const char *template_str, CAXObject *context);
char *cax_template_render_file(const char *tpl_path, CAXObject *context);

#endif