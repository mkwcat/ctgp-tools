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

#ifndef _COMMON_H
#define _COMMON_H

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

typedef struct DISC_INTERFACE DISC_INTERFACE;

typedef uint32_t              sec_t;
typedef uint32_t              _FAT_off_t;
typedef int                   _FAT_ino_t;

typedef struct {
    int   device;
    void* dirStruct;
} DIR_ITER;

struct _FAT_stat {
    int32_t  st_dev;
    int      st_ino;
    uint32_t st_size;
    uint32_t st_mode;
    time_t   st_atime;
    time_t   st_mtime;
    time_t   st_ctime;
    int32_t  st_blocks;
    int32_t  st_blksize;
};

struct _reent {
    int _errno;
};

#define _FAT_S_IFDIR (1u << 0)
#define _FAT_S_IFREG (1u << 1)
#define _FAT_S_IRUSR (1u << 2)
#define _FAT_S_IRGRP (1u << 3)
#define _FAT_S_IROTH (1u << 4)
#define _FAT_S_IWUSR (1u << 5)
#define _FAT_S_IWGRP (1u << 6)
#define _FAT_S_IWOTH (1u << 7)

#define NAME_MAX 1024

// File attributes
#define ATTR_ARCHIVE 0x20   // Archive
#define ATTR_DIRECTORY 0x10 // Directory
#define ATTR_VOLUME 0x08    // Volume
#define ATTR_SYSTEM 0x04    // System
#define ATTR_HIDDEN 0x02    // Hidden
#define ATTR_READONLY 0x01  // Read only

static const unsigned FEATURE_MEDIUM_CANWRITE = 1;

#define DEFAULT_CACHE_PAGES 4
#define DEFAULT_SECTORS_PAGE 64
#define USE_RTC_TIME

#endif // _COMMON_H
