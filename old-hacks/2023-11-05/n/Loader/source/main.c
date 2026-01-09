/*
 * Author : MikeIsAStar
 * Date   : 13 Aug 2023
 * File   : main.c
 */

#include "miniz.h"

#include <ogc/cache.h>
#include <ogc/system.h>

#define CREATE_B_INSTRUCTION(source, destination) \
    ((18 << 26) | (((u32)destination - (u32)source) & 0x03FFFFFC))

#define APPLOADER_ADDRESS 0x81300000
#define DECOMPRESSED_STAGE_1_DOL_ADDRESS 0x80004000

#define PAYLOAD_ADDRESS (u32 *)0x800014B0
#define PAYLOAD_BRANCH_INSTRUCTION_ADDRESS (u32 *)0x800033FC
#define PAYLOAD_BRANCH_INSTRUCTION \
    CREATE_B_INSTRUCTION(PAYLOAD_BRANCH_INSTRUCTION_ADDRESS, PAYLOAD_ADDRESS)

extern u8 apploader[];
extern u32 apploaderSize;

extern u8 compressedStage1Dol[];
extern u32 compressedStage1DolSize;

extern u8 payload[];
extern u32 payloadSize;

u8 sHeap[0x10000];
u32 sNextFreeBlock = 0;

void *zmalloc(void *opaque, size_t items, size_t size) {
    size *= items;
    sNextFreeBlock += size;

    if (sNextFreeBlock > sizeof(sHeap)) {
        exit(EXIT_FAILURE);
    }

    return sHeap + sNextFreeBlock - size;
}

__attribute__((noreturn)) int main(int /* argc */, char ** /* argv */) {
    if (payloadSize > 0x250) {
        exit(EXIT_FAILURE);
    }

    mz_stream mzStream;
    {
        memset(&mzStream, 0, sizeof(mzStream));
        mzStream.next_in = compressedStage1Dol;
        mzStream.avail_in = compressedStage1DolSize;
        mzStream.next_out = (u8 *)DECOMPRESSED_STAGE_1_DOL_ADDRESS;
        mzStream.avail_out = 0x100000;
        mzStream.zalloc = zmalloc;
        mzStream.zfree = Z_NULL;
    }

    if (mz_inflateInit(&mzStream) != MZ_OK) {
        exit(EXIT_FAILURE);
    }
    if (mz_inflate(&mzStream, Z_SYNC_FLUSH) != MZ_STREAM_END) {
        exit(EXIT_FAILURE);
    }

    *PAYLOAD_BRANCH_INSTRUCTION_ADDRESS = PAYLOAD_BRANCH_INSTRUCTION;
    DCFlushRange(PAYLOAD_BRANCH_INSTRUCTION_ADDRESS, sizeof(PAYLOAD_BRANCH_INSTRUCTION));

    memcpy(PAYLOAD_ADDRESS, payload, payloadSize);
    DCFlushRange(PAYLOAD_ADDRESS, payloadSize);

    memcpy((void *)APPLOADER_ADDRESS, apploader, apploaderSize);
    DCFlushRange((void *)APPLOADER_ADDRESS, apploaderSize);

    SYS_ResetSystem(SYS_SHUTDOWN, 0, 0);
    ((void (*)(u32 *))APPLOADER_ADDRESS)((u32 *)DECOMPRESSED_STAGE_1_DOL_ADDRESS);
    __builtin_unreachable();
}
