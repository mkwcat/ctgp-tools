#include "fatfile.h"
#include "partition.h"
#include <stdio.h>

int main(
    int argc, char** argv
) {
    PARTITION* partition = fat_efs_partition_create(argv[1], EFS_TYPE_EFA);
    printf("partition: %p\n", (void*) partition);
    FILE_STRUCT      f;
    struct fat_reent r;
    FILE_STRUCT*     fp = fat_open_r(&r, &f, "efa:/packages.bin", 1);
    printf("fp: %p, errno: %d\n", (void*) fp, r._errno);
}
