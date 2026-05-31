#include "fatfile.h"
#include "partition.h"
#include <stdio.h>

int main(
    int argc, char** argv
) {
    fat_partition* partition = fat_efs_partition_create(argv[1], fat_efs_type_efa);
    printf("partition: %p\n", (void*) partition);
    fat_file         f;
    struct fat_reent r;
    fat_file*        fp = fat_open_r(&r, &f, "efa:/packages.bin", 1);
    printf("fp: %p, errno: %d\n", (void*) fp, r._errno);
}
