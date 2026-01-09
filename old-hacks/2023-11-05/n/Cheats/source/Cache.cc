/*
 * Author : MikeIsAStar
 * Date   : 13 Aug 2023
 * File   : Cache.cc
 */

__asm__ void Cache_WriteInstruction(register unsigned int *instruction,
        register unsigned int newInstruction) {
    nofralloc;

    stw newInstruction, 0(instruction);
    dcbst r0, instruction;
    sync;
    icbi r0, instruction;
    isync;
    blr;
}
