/*
 common.h
 Common definitions and included files for the FATlib

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

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t sec_t;
typedef uint32_t fat_off_t;
typedef int32_t  fat_ino_t;

typedef struct {
    int32_t device;
    void*   dirStruct;
} DIR_ITER;

struct fat_stat {
    int32_t   st_dev;
    fat_ino_t st_ino;
    uint32_t  st_size;
    uint32_t  st_mode;
    time_t    st_atime;
    time_t    st_mtime;
    time_t    st_ctime;
    int32_t   st_blocks;
    int32_t   st_blksize;
};

struct fat_reent {
    int _errno;
};

enum {
    fat_S_IFDIR = (1u << 0),
    fat_S_IFREG = (1u << 1),
    fat_S_IRUSR = (1u << 2),
    fat_S_IRGRP = (1u << 3),
    fat_S_IROTH = (1u << 4),
    fat_S_IWUSR = (1u << 5),
    fat_S_IWGRP = (1u << 6),
    fat_S_IWOTH = (1u << 7),
};

enum {
    fat_NAME_MAX = 1024,
};

// File attributes
enum {
    fat_ATTR_ARCHIVE   = 0x20, // Archive
    fat_ATTR_DIRECTORY = 0x10, // Directory
    fat_ATTR_VOLUME    = 0x08, // Volume
    fat_ATTR_SYSTEM    = 0x04, // System
    fat_ATTR_HIDDEN    = 0x02, // Hidden
    fat_ATTR_READONLY  = 0x01, // Read only
};

enum {
    fat_FEATURE_MEDIUM_CANWRITE = 1,
};

enum {
    fat_DEFAULT_CACHE_PAGES  = 4,
    fat_DEFAULT_SECTORS_PAGE = 64,
};

#ifdef __cplusplus
} // extern "C"
#endif
