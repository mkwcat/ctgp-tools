/*
 disc.h
 Interface to the low level disc functions. Used by the higher level
 file system code.

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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fat_disc fat_disc;

enum fat_efs_type {
    fat_efs_type_efa,
    fat_efs_type_efb,
};

/*
Create an EFS disc with the type
*/
fat_disc* fat_efs_disc_create(const char* path, enum fat_efs_type type);

/*
Destroy an EFS disc created with fat_efs_create
*/
void fat_efs_disc_destroy(fat_disc* disc);

/*
Check if a disc is inserted
Return true if a disc is inserted and ready, false otherwise
*/
static inline bool fat_disc_isInserted(
    fat_disc* disc
) {
    (void) disc;

    return true;
}

/*
Read numSectors sectors from a disc, starting at sector.
numSectors is between 1 and LIMIT_SECTORS if LIMIT_SECTORS is defined,
else it is at least 1
sector is 0 or greater
buffer is a pointer to the memory to fill
*/
bool fat_disc_readSectors(fat_disc* disc, sec_t sector, sec_t numSectors, void* buffer);

/*
Write numSectors sectors to a disc, starting at sector.
numSectors is between 1 and LIMIT_SECTORS if LIMIT_SECTORS is defined,
else it is at least 1
sector is 0 or greater
buffer is a pointer to the memory to read from
*/
bool fat_disc_writeSectors(fat_disc* disc, sec_t sector, sec_t numSectors, const void* buffer);

/*
Reset the card back to a ready state
*/
static inline bool fat_disc_clearStatus(
    fat_disc* disc
) {
    (void) disc;

    return true;
}

/*
Initialise the disc to a state ready for data reading or writing
*/
static inline bool fat_disc_startup(
    fat_disc* disc
) {
    (void) disc;

    return true;
}

/*
Put the disc in a state ready for power down.
Complete any pending writes and disable the disc if necessary
*/
static inline bool fat_disc_shutdown(
    fat_disc* disc
) {
    (void) disc;

    return true;
}

/*
Return a 32 bit value unique to each type of interface
*/
static inline uint32_t fat_disc_hostType(
    fat_disc* disc
) {
    (void) disc;

    return 0;
}

/*
Return a 32 bit value that specifies the capabilities of the disc
*/
static inline uint32_t fat_disc_features(
    fat_disc* disc
) {
    (void) disc;

    return fat_FEATURE_MEDIUM_CANWRITE;
}

#ifdef __cplusplus
} // extern "C"
#endif
