/*
 * Author : MikeIsAStar
 * Date   : 13 Aug 2023
 * File   : Cache.hh
 */

#pragma once

__asm__ void Cache_WriteInstruction(register unsigned int *instruction,
        register unsigned int newInstruction);
