#include "collection.h"
#include "../utils/string_utils.h"
#include <stdlib.h>
#include <string.h>

CAXCollection* cax_collection_new(const char *name) {
    CAXCollection *col = malloc(sizeof(CAXCollection));
    col->name = cax_strdup(name);
    col->items = malloc(sizeof(CAXArray));
    col->items->count = 0;
    col->items->capacity = 8;
    col->items->items = malloc(col->items->capacity * sizeof(CAXObject*));
    return col;
}
void cax_collection_free(CAXCollection *col) {
    if (!col) return;
    free(col->name);
    for (int i = 0; i < col->items->count; i++) cax_object_free(col->items->items[i]);
    free(col->items->items);
    free(col->items);
    free(col);
}
void cax_collection_add_item(CAXCollection *col, CAXObject *item) {
    if (!col ||!item) return;
    if (col->items->count >= col->items->capacity) {
        col->items->capacity *= 2;
        col->items->items = realloc(col->items->items, col->items->capacity * sizeof(CAXObject*));
    }
    col->items->items[col->items->count++] = item;
}