/*
 fatfile.h

 Functions used by the newlib disc stubs to interface with
 this library

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
#include "directory.h"
#include "partition.h"

#ifdef __cplusplus
extern "C" {
#endif

static const uint32_t fat_file_max_size = 0xFFFFFFFFu; // 4GiB - 1B

typedef struct fat_file_position {
    uint32_t cluster;
    sec_t    sector;
    uint32_t byte;
} fat_file_position;

typedef struct fat_file {
    uint32_t          filesize;
    uint32_t          startCluster;
    uint32_t          currentPosition;
    fat_file_position rwPosition;
    fat_file_position appendPosition;
    DIR_ENTRY_POSITION
    dirEntryStart; // Points to the start of the LFN entries of a file, or the alias for no LFN
    DIR_ENTRY_POSITION dirEntryEnd; // Always points to the file's alias entry
    fat_partition*     partition;
    struct fat_file*   prevOpenFile; // The previous entry in a double-linked list of open files
    struct fat_file*   nextOpenFile; // The next entry in a double-linked list of open files
    bool               read;
    bool               write;
    bool               append;
    bool               inUse;
    bool               modified;
} fat_file;

fat_file* fat_open_r(struct fat_reent* r, fat_file* fileStruct, const char* path, int32_t flags);

int32_t fat_close_r(struct fat_reent* r, void* fd);

int32_t fat_write_r(struct fat_reent* r, void* fd, const char* ptr, uint32_t len);

int32_t fat_read_r(struct fat_reent* r, void* fd, char* ptr, uint32_t len);

fat_off_t fat_seek_r(struct fat_reent* r, void* fd, fat_off_t pos, int32_t dir);

int32_t fat_fstat_r(struct fat_reent* r, void* fd, struct fat_stat* st);

int32_t fat_stat_r(struct fat_reent* r, const char* path, struct fat_stat* st);

int32_t fat_link_r(struct fat_reent* r, const char* existing, const char* newLink);

int32_t fat_chdir_r(struct fat_reent* r, const char* name);

int32_t fat_rename_r(struct fat_reent* r, const char* oldName, const char* newName);

int32_t fat_ftruncate_r(struct fat_reent* r, void* fd, fat_off_t len);

int32_t fat_fsync_r(struct fat_reent* r, void* fd);

int32_t fat_getAttr(const char* file);

int32_t fat_setAttr(const char* file, uint8_t attr);

/*
Synchronizes the file data to disc.
Does no locking of its own -- lock the partition before calling.
Returns 0 on success, an error code on failure.
*/
int32_t fat_syncToDisc(fat_file* file);

#ifdef __cplusplus
} // extern "C"
#endif
