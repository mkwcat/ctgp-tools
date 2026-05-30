/*
 disc.c
 Interface to the low level disc functions. Used by the higher level
 file system code.

 Copyright (c) 2008 Michael "Chishm" Chisholm

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

#include "disc.h"

#include "aes.h"
#include "common.h"
#include "mem_allocate.h"
#include <stdio.h>
#include <string.h>

struct DISC_INTERFACE {
    FILE*          file;
    struct AES_ctx aes;
    size_t         sector;
};

static const uint8_t BlobKey[16] = {
    0x90, 0x83, 0x00, 0x04, 0x90, 0xA3, 0x00, 0x08, 0x90, 0xC3, 0x00, 0x0C, 0x4E, 0x80, 0x00, 0x20,
};

static const uint8_t BlobIv[16] = {
    0x80, 0x63, 0x00, 0x04, 0x90, 0x83, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x4E, 0x80, 0x00, 0x20,
};

#define SECTOR_SIZE 512
#define BLOCK_SIZE_SEC 64
#define BLOCK_SIZE (SECTOR_SIZE * BLOCK_SIZE_SEC)

DISC_INTERFACE* _FAT_efs_disc_create(
    const char* path, enum EFS_TYPE type
) {
    FILE* f = fopen(path, "rb");
    if (f == nullptr) {
        return nullptr;
    }

    DISC_INTERFACE* disc = (DISC_INTERFACE*) _FAT_mem_allocate(sizeof(DISC_INTERFACE));
    if (disc == nullptr) {
        fclose(f);
        return nullptr;
    }
    disc->file   = f;
    disc->sector = 0;
    AES_init_ctx(&disc->aes, BlobKey);
    return disc;
}

void _FAT_efs_disc_destroy(
    DISC_INTERFACE* disc
) {
    fclose(disc->file);
    _FAT_mem_free(disc);
}

static uint8_t* blobEncodeIv(
    uint32_t sector, uint8_t* iv
) {
    memcpy(iv, BlobIv, sizeof(BlobIv));
    uint8_t* sectorVal = (uint8_t*) &sector;
    iv[8]              = sectorVal[0];
    iv[9]              = sectorVal[1];
    iv[10]             = sectorVal[2];
    iv[11]             = sectorVal[3];
    return iv;
}

bool _FAT_disc_readSectors(
    DISC_INTERFACE* disc, sec_t sector, sec_t numSectors, void* buffer
) {
    uint8_t iv[16];
    if ((sector % BLOCK_SIZE_SEC) == 0) {
        if (fseek(disc->file, sector * SECTOR_SIZE, SEEK_SET) != 0) {
            return false;
        }
        AES_ctx_set_iv(&disc->aes, blobEncodeIv(sector, iv));
    } else if (sector != disc->sector) {
        // Read IV from previous 16 bytes
        if (fseek(disc->file, (sector * SECTOR_SIZE) - 16, SEEK_SET) != 0 ||
            fread(iv, 1, 16, disc->file) != 16) {
            return false;
        }
        AES_ctx_set_iv(&disc->aes, iv);
    }
    // Else the AES context is already prepared for the next data

    if (fread(buffer, SECTOR_SIZE, numSectors, disc->file) != numSectors) {
        return false;
    }

    uint8_t* data = (uint8_t*) buffer;
    for (uint32_t s = 0; s < numSectors;) {
        if (s != 0) {
            AES_ctx_set_iv(&disc->aes, blobEncodeIv(sector + s, iv));
        }

        uint32_t blockEnd     = BLOCK_SIZE_SEC - ((sector + s) % BLOCK_SIZE_SEC);
        uint32_t decryptCount = numSectors < blockEnd ? numSectors - s : blockEnd - s;

        AES_CBC_decrypt_buffer(&disc->aes, data + s * SECTOR_SIZE, decryptCount * SECTOR_SIZE);
        s += decryptCount;
    }
    disc->sector = sector + numSectors;

    return true;
}

bool _FAT_disc_writeSectors(
    DISC_INTERFACE* disc, sec_t sector, sec_t numSectors, const void* buffer
) {
    return false;
}
