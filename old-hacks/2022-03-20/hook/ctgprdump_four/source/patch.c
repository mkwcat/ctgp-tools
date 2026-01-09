#include <gctypes.h>
#include <stdio.h>
#include <string.h>
#include <ogc/cache.h>
#include <ogc/machine/processor.h>

// fopen = 809b4b7c
// fseek = 809b54b4 NOPE
// ftell = 809b5b30
// malloc = 809b6a1c

const char except[] = "sda:/ctgpr/CTGPException.bin";

inline uint32_t align_forward(uint32_t value, uint32_t align)
{
    return (value + align - 1) & (-1 ^ (align - 1));
}

inline uint32_t align_backward(uint32_t value, uint32_t align)
{
    return value & (-1 ^ (align - 1));
}

/* from rmc-local-net */
#define POKE_B(addr, dest)                                                     \
    do {                                                                       \
        *(uint32_t*) (addr)                                                    \
            = 0x48000000                                                       \
              + (((uint32_t)(dest) - (uint32_t)(addr)) & 0x3ffffff);           \
        DCFlushRange((void*) align_backward(addr, 32), 32);                    \
        ICInvalidateRange((void*) align_backward(addr, 32), 32);               \
    } while (0)
#define POKE_BL(addr, dest)                                                    \
    do {                                                                       \
        *(u32*) (addr)                                                    \
            = 0x48000001                                                       \
              + (((u32)(dest) - (u32)(addr)) & 0x3ffffff);           \
        DCFlushRange((void*) align_backward(addr, 32), 32);                    \
        ICInvalidateRange((void*) align_backward(addr, 32), 32);               \
    } while (0)

u8 fopenSearch[] = { 0x7C, 0x85, 0x23, 0x78, 0x7D, 0x24, 0x4B, 0x78, 0x4B, 0xFF, 0xFE, 0x64, 0x7C, 0x08, 0x02, 0xA6 };

u8 dolphinProtSearch[] = { 0x38, 0xA0, 0x00, 0x00, 0x38, 0xC0, 0x00, 0x00, 0x60, 0xE7, 0x80, 0x00, 0x39, 0x00, 0x00, 0x03, 0x3B, 0x20, 0x00, 0x05 };

/* A screen transition function of some sort. */
u8 launchGameSymSearch[] = {0x4B, 0xFF, 0xFF, 0xB4, 0x7C, 0x08, 0x02, 0xA6, 0x94, 0x21, 0xFF, 0xF8, 0x38, 0x60, 0x00, 0x00, 0x90, 0x01, 0x00, 0x0C};

//808788e0

FILE* log_file;
u32 fopen_adr;
u32 fread_adr;
u32 fwrite_adr;
u32 fclose_adr;
u32 launchGameSym = 0;

FILE* ct_fopen(const char* name, const char* mode)
{
    return ((FILE* (*)(const char*, const char*)) fopen_adr)(name, mode);
}




u32 search(u32 start, u32 end, const u8* term, u32 size)
{
    for (u32 c = start; c < end; c += 4)
    {
        if (!memcmp((void*) c, (void*) term, size))
            return c;
    }
    return 0;
}

extern u8 ctgp_hook_patch[];
extern u32 ctgp_hook_patch_size;

extern u8 codehandler_bin[];
extern u32 codehandler_bin_size;

extern u8 RMCE01_gct[];
extern u32 RMCE01_gct_size;
extern u8 RMCP01_gct[];
extern u32 RMCP01_gct_size;

u8 antiCodehandlerSearch[] = { 0x94, 0x21, 0xFF, 0xF0, 0x7C, 0x08, 0x02, 0xA6, 0x90, 0x01, 0x00, 0x14, 0x7D, 0x40, 0x00, 0xA6, 0x3C, 0xA0, 0xFF, 0xFF, 0x60, 0xA5, 0xFF, 0xEF, 0x7C, 0xAA, 0x50, 0x38 };

const u8 MapMMUSearch[] = { 0x4E, 0x80, 0x04, 0x21, 0x48, 0x00, 0x00, 0x00, 0x38, 0x80, 0x00, 0x03, 0x4B, 0xFF, 0xFF, 0x68, 0x7C, 0x08, 0x02, 0xA6, 0x94, 0x21, 0xFF, 0xF0, 0x93, 0xE1, 0x00, 0x0C, 0x7C, 0x7F, 0x1B, 0x78 };

void xor_codehandler()
{
    for (int i = 0; i < codehandler_bin_size; i++) {
        codehandler_bin[i] ^= 0xFF;
    }
}

void rk_entry_point()
{
    u8* gct;
    u32 gct_size;

    u8 region = *(u8*) 0x80000003;

    if (region == 'E') {
        gct = RMCE01_gct;
        gct_size = RMCE01_gct_size;
    } else if (region == 'P') {
        gct = RMCP01_gct;
        gct_size = RMCP01_gct_size;
    } else {
        return;
    }

    //*(u32*) 0x801264FC = 0x4807D3B0; // branch to 0x801A38AC

    //u32 anti_codehandler = search(0x817DD000, 0x817E0000, antiCodehandlerSearch, sizeof(antiCodehandlerSearch));
    //*(u32*) anti_codehandler = 0x4E800020;

    u32 arena_hi = *(u32*) 0x80000034;
    u32 codehandler_addr = (arena_hi - codehandler_bin_size);

    xor_codehandler();
    memcpy((void*) codehandler_addr, (void*) codehandler_bin, codehandler_bin_size);

    /* Relocations */
    *(u32*) (codehandler_addr + 8) = codehandler_addr + 0xF84;
    *(u16*) (codehandler_addr + 0xF2) = (codehandler_addr - 0x1800) >> 16;
    *(u16*) (codehandler_addr + 0xF6) = (codehandler_addr - 0x1800) & 0xFFFF;
    *(u16*) (codehandler_addr + 0x2CE) = (codehandler_addr - 0x7FFFF040 + 0x8000) >> 16;
    *(u16*) (codehandler_addr + 0x2D2) = (codehandler_addr - 0x7FFFF040) & 0xFFFF;
    *(u16*) (codehandler_addr + 0x2E6) = (codehandler_addr + 0x2F4) >> 16;
    *(u16*) (codehandler_addr + 0x2EA) = (codehandler_addr + 0x2F4) & 0xFFFF;
    *(u16*) (codehandler_addr + 0x2F6) = (codehandler_addr + 0xFA8) >> 16;
    *(u16*) (codehandler_addr + 0x2FA) = (codehandler_addr + 0xFA8) & 0xFFFF;
    *(u16*) (codehandler_addr + 0x35A) = (codehandler_addr + 0x37C) >> 16;
    *(u16*) (codehandler_addr + 0x35E) = (codehandler_addr + 0x37C) & 0xFFFF;
    *(u16*) (codehandler_addr + 0x392) = (codehandler_addr - 0x1800) >> 16;
    *(u16*) (codehandler_addr + 0x396) = (codehandler_addr - 0x1800) & 0xFFFF;

    u32 codelist_addr = codehandler_addr - gct_size;
    
    *(u16*) (codehandler_addr + 0x4EE) = (codelist_addr) >> 16;
    *(u16*) (codehandler_addr + 0x4F2) = (codelist_addr) & 0xFFFF;
    *(u16*) (codehandler_addr + 0x76A) = (codelist_addr) >> 16;
    *(u16*) (codehandler_addr + 0x76E) = (codelist_addr) & 0xFFFF;

    DCFlushRange((void*) codehandler_addr, codehandler_bin_size);

    

    memcpy((void*) codelist_addr, (void*) gct, gct_size);
    DCFlushRange((void*) codelist_addr, gct_size);

    *(u32*) 0x80000034 = codelist_addr - 0x1000;
    *(u32*) 0x80003110 = codelist_addr - 0x1000;

    
    if (region == 'E')
        POKE_B(0x801264FC, codehandler_addr + 0xA8);
    else if (region == 'P')
        POKE_B(0x8012659C, codehandler_addr + 0xA8);

    /* This disables the memory mapping that breaks our codehandler. */
    u32 mmufunc = search(0x817B0000, 0x817D0000, MapMMUSearch, sizeof(MapMMUSearch)) + 0x10;
    *(u32*) mmufunc = 0x4E800020;
    DCFlushRange((void*) mmufunc, 4);

    //memcpy((void*) 0x801A38AC, (void*) ctgp_hook_patch, ctgp_hook_patch_size);
}


void do_searches()
{
    fopen_adr = search(0x80800000, 0x80b00000, fopenSearch, sizeof(fopenSearch));
    if (fopen_adr != 0) {
        fopen_adr -= 0xC;
    }
    launchGameSym = search(0x80800000, 0x80b00000, launchGameSymSearch, sizeof(launchGameSymSearch));
    launchGameSym += 4;
    *(u32*)(launchGameSym + 0x48) = 0x7D2803A6;
    POKE_B(launchGameSym + 0x4C, rk_entry_point);

    u32 dolphin_prot = search(0x80900000, 0x80940000, dolphinProtSearch, sizeof(dolphinProtSearch));
    if (dolphin_prot) {
        dolphin_prot += 0x80;
        *(u32*) dolphin_prot = 0x60000000;
        DCFlushRange((void*) dolphin_prot, 4);
    }

    u32 exception_bin_str = search(0x809F5000, 0x809FC000, (u8*)except, sizeof(except));
    if (exception_bin_str) {
        *(u8*) exception_bin_str = 0;
    }
}


void patch_channel()
{
    xor_codehandler();
    do_searches();
}