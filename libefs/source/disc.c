#include "disc.h"

#include "aes.h"
#include "common.h"
#include "mem_allocate.h"
#include <stdio.h>
#include <string.h>

struct fat_disc {
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

enum {
    SECTOR_SIZE    = 512,
    BLOCK_SIZE_SEC = 64,
    BLOCK_SIZE     = (SECTOR_SIZE * BLOCK_SIZE_SEC),
};

fat_disc* fat_efs_disc_create(
    const char* path, enum efs_type type
) {
    (void) type;

    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        return NULL;
    }

    fat_disc* disc = (fat_disc*) fat_mem_allocate(sizeof(fat_disc));
    if (disc == NULL) {
        fclose(f);
        return NULL;
    }
    disc->file   = f;
    disc->sector = 0;
    AES_init_ctx(&disc->aes, BlobKey);
    return disc;
}

void fat_efs_disc_destroy(
    fat_disc* disc
) {
    fclose(disc->file);
    fat_mem_free(disc);
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

static void printHex(
    const uint8_t* p, size_t n
) {
    for (size_t i = 0; i < n; i++) {
        printf("%02X ", p[i]);
        if (((i + 1) % 16) == 0) {
            puts("");
        }
    }
}

bool fat_disc_readSectors(
    fat_disc* disc, sec_t sector, sec_t numSectors, void* buffer
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

#if 0
    printf("sector dump @ %u\n", sector);
    printHex((uint8_t*) buffer, numSectors * SECTOR_SIZE);
#else
    (void) printHex;
#endif
    return true;
}

bool fat_disc_writeSectors(
    fat_disc* disc, sec_t sector, sec_t numSectors, const void* buffer
) {
    (void) disc;
    (void) sector;
    (void) numSectors;
    (void) buffer;

    return false;
}
