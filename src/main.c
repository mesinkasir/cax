#include <stdio.h>
#include <string.h>
#include "build/builder.h"
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("\n CAX Enterprise SSG\n cax init\n cax build\n cax serve\n cax start\n\n"); return 0;
    }
    if (strcmp(argv[1], "init") == 0) { cax_builder_init_project(); }
    else if (strcmp(argv[1], "build") == 0) { cax_builder_run(); }
    else if (strcmp(argv[1], "serve") == 0) { cax_builder_serve(); }
    else if (strcmp(argv[1], "start") == 0 || strcmp(argv[1], "watch") == 0) { cax_builder_start_with_watch(); }
    else { printf(" [ERROR] Unknown: %s\n", argv[1]); }
    return 0;
}