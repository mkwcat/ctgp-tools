#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

enum efs_type {
    efs_type_efa,
    efs_type_efb,
};

bool efs_mount(const char* path, enum efs_type type);

typedef struct fat_dir_iter {
    int32_t device;
    void*   dirStruct;
} fat_dir_iter;

typedef struct fat_stat {
    int32_t  st_dev;
    int32_t  st_ino;
    uint32_t st_size;
    uint32_t st_mode;
    time_t   st_atime;
    time_t   st_mtime;
    time_t   st_ctime;
    int32_t  st_blocks;
    int32_t  st_blksize;
} fat_stat;

void* efs_open(int* error, const char* path, int32_t flags);

struct fat_file*
efs_open_r(int* error, struct fat_file* fileStruct, const char* path, int32_t flags);

int efs_close(int* error, void* fd);

int32_t efs_write(int* error, void* fd, const char* ptr, uint32_t len);

int32_t efs_read(int* error, void* fd, char* ptr, uint32_t len);

uint32_t efs_seek(int* error, void* fd, uint32_t pos, int32_t dir);

int efs_fstat(int* error, void* fd, struct fat_stat* st);

int efs_stat(int* error, const char* path, struct fat_stat* st);

int efs_link(int* error, const char* existing, const char* newLink);

int efs_unlink(int* error, const char* name);

int efs_chdir(int* error, const char* name);

int efs_rename(int* error, const char* oldName, const char* newName);

int efs_ftruncate(int* error, void* fd, uint32_t len);

int efs_fsync(int* error, void* fd);

int efs_getAttr(const char* file);

int efs_setAttr(const char* file, uint8_t attr);

int efs_mkdir(int* error, const char* path);

int efs_rmdir(int* error, const char* path);

/*
 Directory iterator functions
*/
fat_dir_iter* efs_diropen(int* error, fat_dir_iter* dirState, const char* path);
int efs_dirreset(int* error, fat_dir_iter* dirState);
int efs_dirnext(int* error, fat_dir_iter* dirState, char* filename, struct fat_stat* filestat);
int efs_dirclose(int* error, fat_dir_iter* dirState);
