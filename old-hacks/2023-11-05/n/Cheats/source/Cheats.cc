/*
 * Author : MikeIsAStar
 * Date   : 13 Aug 2023
 * File   : Cheats.cc
 */

#include "Cache.hh"
#include "String.hh"

extern "C" {
#include <revolution/os.h>
}

typedef void (*SignatureFoundCallback)(void *signature);

typedef struct CheatPatch {
    void *signature;
    unsigned int signatureLength;
    SignatureFoundCallback signatureFoundCallback;
} CheatPatch;

namespace MyStuffSetting {
enum {
    OFF,
    ON,
    NO_REPLACED_TRACKS,
    BRSTMS_ONLY,
};
}

static unsigned int sBlockCommonSZSSubFilesFunctionSignature[] = {
        0x9421FF68, // stwu      r1, -0x98(r1)
        0x7C0802A6, // mfspr     r0, LR
        0xBF810088, // stmw      r28, 0x98+var_10(r1)
        0x7C9E2378, // mr        r30, r4
};

static void UnblockCommonSZSSubFiles(void *blockCommonSZSSubFilesFunction) {
    int iEnabled = OSDisableInterrupts();
    {
        // clang-format off
        unsigned int b = 0x4800008C; // b 0x8C
        Cache_WriteInstruction(reinterpret_cast<unsigned int *>(blockCommonSZSSubFilesFunction) + 0x01A,
                           b); // /ItemSlot.bin
                           Cache_WriteInstruction(reinterpret_cast<unsigned int *>(blockCommonSZSSubFilesFunction) + 0x22F,
                           b); // /kartParam.bin
        // clang-format on
    }
    OSRestoreInterrupts(iEnabled);
}

static const char sWC24ScrVffFilepath[] = "4b/data/wc24scr";

static void SpoofMyStuffSetting(void *wc24ScrVffFilepath) {
    *reinterpret_cast<unsigned int *>((reinterpret_cast<char *>(wc24ScrVffFilepath) - 0x59A)) =
            MyStuffSetting::OFF;
}

static void TryDoCheatPatch(CheatPatch *cheatPatch) {
    char *haystackStart = *reinterpret_cast<char **>(MEM1_ARENA_HI);
    char *haystackEnd = reinterpret_cast<char *>(MEM1_END);

    void *signature = memmem(haystackStart, haystackEnd - haystackStart, cheatPatch->signature,
            cheatPatch->signatureLength);

    if (signature) {
        cheatPatch->signatureFoundCallback(signature);
    }
}

static CheatPatch cheatPatchArray[] = {
        // clang-format off
        {
            (void *)sBlockCommonSZSSubFilesFunctionSignature,
            sizeof(sBlockCommonSZSSubFilesFunctionSignature),
            UnblockCommonSZSSubFiles,
        },
        {
            (void *)sWC24ScrVffFilepath,
            sizeof(sWC24ScrVffFilepath) -1,
            SpoofMyStuffSetting,
        },
        // clang-format on
};

void Cheats_DoCheatPatches() {
    for (unsigned int n = 0; n < sizeof(cheatPatchArray) / sizeof(CheatPatch); n++) {
        TryDoCheatPatch(&cheatPatchArray[n]);
    }
}
