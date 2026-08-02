#ifndef COLLECTION_H
#define COLLECTION_H

#include "../../include/cax.h"

typedef struct {
    char *name;
    CAXArray *items;
} CAXCollection;

CAXCollection* cax_collection_new(const char *name);
void cax_collection_free(CAXCollection *col);
void cax_collection_add_item(CAXCollection *col, CAXObject *item);

#endif