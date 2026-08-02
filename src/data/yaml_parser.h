#ifndef YAML_PARSER_H
#define YAML_PARSER_H
#include "../../include/cax.h"

CAXObject* cax_parse_yaml(const char *yaml_str);
void cax_load_yaml_file(const char *filepath, CAXObject *global_ctx);

#endif