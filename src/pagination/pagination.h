#ifndef CAX_PAGINATION_H
#define CAX_PAGINATION_H
#include "../data/data_manager.h"
void cax_pagination_process(CAXArray *coll_array, int per_page, const char *out_dir, const char *tpl, CAXObject *global_data, CAXObject *pagination_controllers, const char *coll_name);
#endif