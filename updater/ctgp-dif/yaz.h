#pragma once

#include "types.h"
#include <cstdio>

#define BACK_BUF_SIZE 0x1000
#define HASH_MAP_SIZE 0x4000

struct Yaz_file_struct
{
    FILE* file;
    u32 _0x4;
    int iteration;
    u32 backBufferLocation; // C
    u8 backBuffer[BACK_BUF_SIZE];
    u32 field5_0x1010;
    u32 byteShifterRemaining;
    u32 field7_0x1018;
    u16 hashMap[HASH_MAP_SIZE]; // 101c
    int copyLocation; // 901c
    u32 copyDistance; // 9020
    u32 copyLength;
    u32 field12_0x9028;
    u8 field13_0x902c[0x922C - 0x902C];
    u8 codeBufferLocation; // 922c
    u8 codeBufferIndex; // 922d
    u8 codeBuffer[0x9248 - 0x922E]; // not actual size probably
};

bool Yaz_open(Yaz_file_struct* file, FILE* fptr);
bool Yaz_write(Yaz_file_struct* file, u8* data, u32 len);

struct Ret
{
    int srcPos, dstPos;
};

Ret decodeYaz0(u8* src, int srcSize, u8* dst, int uncompressedSize);
