#ifndef FRONTMATTER_H
#define FRONTMATTER_H

#include "../data/data_manager.h"

typedef struct {
    CAXObject *metadata;
    char *body;
} CAXParsedDocument;

CAXParsedDocument* cax_frontmatter_parse(const char *file_content);
void cax_frontmatter_free(CAXParsedDocument *doc);

#endif