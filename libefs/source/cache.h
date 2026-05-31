/*
 cache.h
 The cache is not visible to the user. It should be flushed
 when any file is closed or changes are made to the filesystem.

 This cache implements a least-used-page replacement policy. This will
 distribute sectors evenly over the pages, so if less than the maximum
 pages are used at once, they should all eventually remain in the cache.
 This also has the benefit of throwing out old sectors, so as not to keep
 too many stale pages around.

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

typedef struct CACHE_ENTRY {
    sec_t    sector;
    uint32_t count;
    uint32_t last_access;
    bool     dirty;
    uint8_t* cache;
} CACHE_ENTRY;

typedef struct CACHE {
    DISC_INTERFACE* disc;
    sec_t           endOfPartition;
    uint32_t        numberOfPages;
    uint32_t        sectorsPerPage;
    uint32_t        bytesPerSector;
    CACHE_ENTRY*    cacheEntries;
} CACHE;

/*
Read data from a sector in the cache
If the sector is not in the cache, it will be swapped in
offset is the position to start reading from
size is the amount of data to read
Precondition: offset + size <= BYTES_PER_READ
*/
bool fat_cache_readPartialSector(
    CACHE* cache, void* buffer, sec_t sector, uint32_t offset, size_t size
);

bool fat_cache_readLittleEndianValue(
    CACHE* cache, uint32_t* value, sec_t sector, uint32_t offset, int32_t num_bytes
);

/*
Write data to a sector in the cache
If the sector is not in the cache, it will be swapped in.
When the sector is swapped out, the data will be written to the disc
offset is the position to start writing to
size is the amount of data to write
Precondition: offset + size <= BYTES_PER_READ
*/
bool fat_cache_writePartialSector(
    CACHE* cache, const void* buffer, sec_t sector, uint32_t offset, size_t size
);

bool fat_cache_writeLittleEndianValue(
    CACHE* cache, const uint32_t value, sec_t sector, uint32_t offset, int32_t num_bytes
);

/*
Write data to a sector in the cache, zeroing the sector first
If the sector is not in the cache, it will be swapped in.
When the sector is swapped out, the data will be written to the disc
offset is the position to start writing to
size is the amount of data to write
Precondition: offset + size <= BYTES_PER_READ
*/
bool fat_cache_eraseWritePartialSector(
    CACHE* cache, const void* buffer, sec_t sector, uint32_t offset, size_t size
);

/*
Read several sectors from the cache
*/
bool fat_cache_readSectors(CACHE* cache, sec_t sector, sec_t numSectors, void* buffer);

/*
Read a full sector from the cache
*/
static inline bool fat_cache_readSector(
    CACHE* cache, void* buffer, sec_t sector
) {
    return fat_cache_readPartialSector(cache, buffer, sector, 0, cache->bytesPerSector);
}

/*
Write a full sector to the cache
*/
static inline bool fat_cache_writeSector(
    CACHE* cache, const void* buffer, sec_t sector
) {
    return fat_cache_writePartialSector(cache, buffer, sector, 0, cache->bytesPerSector);
}

bool fat_cache_writeSectors(CACHE* cache, sec_t sector, sec_t numSectors, const void* buffer);

/*
Write any dirty sectors back to disc and clear out the contents of the cache
*/
bool fat_cache_flush(CACHE* cache);

/*
Clear out the contents of the cache without writing any dirty sectors first
*/
void fat_cache_invalidate(CACHE* cache);

CACHE* fat_cache_constructor(
    uint32_t numberOfPages, uint32_t sectorsPerPage, DISC_INTERFACE* discInterface,
    sec_t endOfPartition, uint32_t bytesPerSector
);

void fat_cache_destructor(CACHE* cache);
