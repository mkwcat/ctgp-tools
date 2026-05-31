/*
 partition.h
 Functions for mounting and dismounting partitions
 on various block devices.

 Copyright (c) 2006 Michael "Chishm" Chisholm

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation and/or
     other materials provided with the distribution.
  3. The name of the author may not be used to endorse or promote products derived
     from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "common.h"
#include "disc.h"
#include "lock.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    MIN_SECTOR_SIZE = 512,
    MAX_SECTOR_SIZE = 4096,
};

// Filesystem type
typedef enum fat_fs_type {
    FS_UNKNOWN,
    FS_FAT12,
    FS_FAT16,
    FS_FAT32,
} fat_fs_type;

typedef struct {
    sec_t    fatStart;
    uint32_t sectorsPerFat;
    uint32_t lastCluster;
    uint32_t firstFree;
    uint32_t numberFreeCluster;
    uint32_t numberLastAllocCluster;
} FAT;

typedef struct fat_partition {
    fat_disc*         disc;
    struct fat_cache* cache;
    // Info about the partition
    fat_fs_type filesysType;
    uint64_t    totalSize;
    sec_t       rootDirStart;
    uint32_t    rootDirCluster;
    uint32_t    numberOfSectors;
    sec_t       dataStart;
    uint32_t    bytesPerSector;
    uint32_t    sectorsPerCluster;
    uint32_t    bytesPerCluster;
    uint32_t    fsInfoSector;
    FAT         fat;
    // Values that may change after construction
    uint32_t         cwdCluster; // Current working directory cluster
    int              openFileCount;
    struct fat_file* firstOpenFile; // The start of a linked list of files
    fat_mutex_t      lock;          // A lock for partition operations
    bool             readOnly;      // If this is set, then do not try writing to the disc
    char             label[12];     // Volume label
} fat_partition;

/*
Mount the supplied device and return a pointer to the struct necessary to use it
*/
fat_partition* fat_partition_constructor(
    fat_disc* disc, uint32_t cacheSize, uint32_t SectorsPerPage, sec_t startSector
);

/*
Dismount the device and free all structures used.
Will also attempt to synchronise all open files to disc.
*/
void fat_partition_destructor(fat_partition* partition);

/*
Return the partition specified in a path, as taken from the devoptab.
*/
fat_partition* fat_partition_getPartitionFromPath(const char* path);

/*
Create the fs info sector.
*/
void fat_partition_createFSinfo(fat_partition* partition);

/*
Read the fs info sector data.
*/
void fat_partition_readFSinfo(fat_partition* partition);

/*
Write the fs info sector data.
*/
void fat_partition_writeFSinfo(fat_partition* partition);

/*
Create a partition and disc for the specified EFS type.
*/
fat_partition* fat_efs_partition_create(const char* path, enum efs_type type);

#ifdef __cplusplus
} // extern "C"
#endif
