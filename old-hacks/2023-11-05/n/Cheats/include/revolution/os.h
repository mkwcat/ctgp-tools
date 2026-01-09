#pragma once

#define OS_CACHED_BASE 0x80000000

#define MEM1_ARENA_HI 0x80000034
#define MEM1_END 0x81800000

void OSReport(const char *msg, ...);

#include "revolution/os/OSInterrupt.h"
