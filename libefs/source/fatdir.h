/*
 fatdir.h

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

#include "common.h"
#include "directory.h"

typedef struct {
    PARTITION* partition;
    DIR_ENTRY  currentEntry;
    uint32_t   startCluster;
    bool       inUse;
    bool       validEntry;
} DIR_STATE_STRUCT;

extern int fat_stat_r(struct fat_reent* r, const char* path, struct fat_stat* st);

extern int fat_link_r(struct fat_reent* r, const char* existing, const char* newLink);

extern int fat_unlink_r(struct fat_reent* r, const char* name);

extern int fat_chdir_r(struct fat_reent* r, const char* name);

extern int fat_rename_r(struct fat_reent* r, const char* oldName, const char* newName);

extern int fat_mkdir_r(struct fat_reent* r, const char* path);

extern int fat_rmdir_r(struct fat_reent* r, const char* path);

#if 0
extern int fat_statvfs_r(struct fat_reent* r, const char* path, struct statvfs* buf);
#endif

/*
Directory iterator functions
*/
extern DIR_ITER* fat_diropen_r(struct fat_reent* r, DIR_ITER* dirState, const char* path);
extern int fat_dirreset_r(struct fat_reent* r, DIR_ITER* dirState);
extern int
fat_dirnext_r(struct fat_reent* r, DIR_ITER* dirState, char* filename, struct fat_stat* filestat);
extern int fat_dirclose_r(struct fat_reent* r, DIR_ITER* dirState);
