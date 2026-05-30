#include "fatfile.h"
#include "partition.h"
#include <stdio.h>

int main(
    int argc, char** argv
) {
    PARTITION* partition = _FAT_efs_partition_create(argv[1], EFS_TYPE_EFA);
    printf("partition: %p\n", (void*) partition);
    FILE_STRUCT   f;
    struct _reent r;
    FILE_STRUCT*  fp = _FAT_open_r(&r, &f, "efa:/packages.bin", 1, 1);
    printf("fp: %p\n", (void*) fp);
}
