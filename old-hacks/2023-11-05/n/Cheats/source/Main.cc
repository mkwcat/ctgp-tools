/*
 * Author : MikeIsAStar
 * Date   : 13 Aug 2023
 * File   : Main.cc
 */

#include "Main.hh"

#include "Cheats.hh"

extern "C" {
#include <revolution/dvd.h>
#include <revolution/os.h>
}

#define DVD_LOW_READ_STACK_SIZE 0x10

#define OVERWRITTEN_FUNCTION_POINTER_ADDRESS reinterpret_cast<unsigned int **>(0x81640000)

static bool RealMain(void *destinationAddress, int length, int offset, void (*busyStateCallback)());

static bool sApplyCheats = true;

static inline void FixOverwrittenFunctionPointer() {
    *OVERWRITTEN_FUNCTION_POINTER_ADDRESS = reinterpret_cast<unsigned int *>(DVDLowRead);
}

__asm__ static void Main(void *destinationAddress, int length, int offset,
        void (*busyStateCallback)()) {
    nofralloc;

    addi r1, r1, DVD_LOW_READ_STACK_SIZE;
    b RealMain
}

static bool RealMain(void *destinationAddress, int length, int offset,
        void (*busyStateCallback)()) {
    __asm__ __volatile__ {
        opword 0x9421FFF0; /* stwu r1, -0x10(r1) */
    }
    bool bSuccess = DVDLowRead(destinationAddress, length, offset, busyStateCallback);

    FixOverwrittenFunctionPointer();
    OSReport("<<%*cMikeIsAStar%*c>>\n", 29, ' ', 28, ' ');

    if (sApplyCheats) {
        Cheats_DoCheatPatches();
        sApplyCheats = false;
    }

    return bSuccess;
}
