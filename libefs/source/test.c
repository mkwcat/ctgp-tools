#include "efs.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef LIBEFS_MAIN

bool make_new_file(
    const char* path, uint8_t* data, size_t size
) {
    int   error;
    void* fp = efs_open(&error, path, O_CREAT | O_WRONLY);
    if (!fp) {
        printf("create new file [%s] failed, error: %d\n", path, error);
        return false;
    }

    if (efs_write(&error, fp, data, size) != size) {
        printf("write new file [%s] failed, error: %d\n", path, error);
        return false;
    }

    if (efs_close(&error, fp) != 0) {
        printf("close new file [%s] failed, error: %d\n", path, error);
        return false;
    }

    return true;
}

int main(
    int argc, char** argv
) {
    if (argc < 2) {
        printf("%s <path/to/blob.bin | path/to/jelly.bin>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (!efs_mount(argv[1], efs_type_efa)) {
        printf("failed to create blob.bin partition\n");
        return EXIT_FAILURE;
    }
    int   error;
    void* fp = efs_open(&error, "efa:/packages.bin", 0);
    if (!fp) {
        printf("open packages.bin failed, error: %d\n", error);
        return EXIT_FAILURE;
    }

    fat_stat stats;
    if (efs_fstat(&error, fp, &stats) != 0) {
        printf("stat packages.bin failed, error: %d\n", error);
        return EXIT_FAILURE;
    }

    char magic[5];
    magic[4] = 0;
    if (efs_read(&error, fp, magic, 4) != 4) {
        printf("read packages.bin failed, error: %d\n", error);
        return EXIT_FAILURE;
    }

    printf("%s\n", magic);
    printf("packages.bin size: 0x%X\n", (unsigned int) stats.st_size);

    uint8_t data[] = {0x4a, 0x50, 0x35, 0x35, 0x03, 0x04, 0x00, 0x04,
                      0x08, 0x29, 0x20, 0x03, 0x00, 0x00, 0x00, 0x10};

    // if (!make_new_file("efa:/johnp55.bin", data)) return EXIT_FAILURE;
    // efs_unlink(&error, "efa:/johnp55longerwhufioewrhguieowr.bin");
    if (!make_new_file("efa:/johnp55longerwhufioewrhguieowr.bin", data, sizeof(data))) {
        return EXIT_FAILURE;
    }
    if (!make_new_file("efa:/johnp55longerwhufioewrhgumeowr.bin", data, sizeof(data))) {
        return EXIT_FAILURE; // todo: figure out why this doesn't get made
    }
    if (!make_new_file("efa:/johnp56.bin", data, sizeof(data))) {
        return EXIT_FAILURE;
    }
    if (!make_new_file("efa:/JOHNP57.BIN", data, sizeof(data))) {
        return EXIT_FAILURE; // todo: figure out why this doesn't get made
    }
    if (!make_new_file("efa:/JOHNP58.BIN", data, sizeof(data))) {
        return EXIT_FAILURE;
    }

    if (efs_close(&error, fp) != 0) {
        printf("close packages.bin failed, error: %d\n", error);
        return EXIT_FAILURE;
    }

    if (!efs_unmount(efs_type_efa)) {
        printf("unmount failed, error: %d\n", error);
        return EXIT_FAILURE;
    }
}

#endif
