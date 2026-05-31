#include "efs.h"
#include <stdio.h>
#include <stdlib.h>

int main(
    int argc, char** argv
) {
    (void) argc;

    if (!efs_mount(argv[1], efs_type_efa)) {
        printf("failed to create blob.bin partition\n");
    }
    int   error;
    void* fp = efs_open(&error, "efa:/packages.bin", 1);
    if (!fp) {
        printf("open packages.bin failed, error: %d\n", error);
        return EXIT_FAILURE;
    }

    fat_stat stats;
    if (efs_fstat(&error, fp, &stats) != 0) {
        printf("stat packages.bin failed, error: %d\n", error);
        return EXIT_FAILURE;
    }

    printf("packages.bin size: 0x%X\n", (unsigned int) stats.st_size);

    if (efs_close(&error, fp) != 0) {
        printf("close packages.bin failed, error: %d\n", error);
        return EXIT_FAILURE;
    }
}
