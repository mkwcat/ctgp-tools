#include <gctypes.h>
#include <ogc/cache.h>
#include <string.h>

#define POKE32(addr, value)                                                    \
    do                                                                         \
    {                                                                          \
        *(uint32_t*)(addr) = (uint32_t)(value);                                \
        DCFlushRange((void*)addr, 4);                                          \
        ICInvalidateRange((void*)addr, 4);                                     \
    } while (0)

/* from rmc-local-net */
#define POKE_B(addr, dest)                                                     \
    do                                                                         \
    {                                                                          \
        *(uint32_t*)(addr) =                                                   \
            0x48000000 + (((uint32_t)(dest) - (uint32_t)(addr)) & 0x3ffffff);  \
        DCFlushRange((void*)addr, 4);                                          \
        ICInvalidateRange((void*)addr, 4);                                     \
    } while (0)
#define POKE_BL(addr, dest)                                                    \
    do                                                                         \
    {                                                                          \
        *(u32*)(addr) =                                                        \
            0x48000001 + (((u32)(dest) - (u32)(addr)) & 0x3ffffff);            \
        DCFlushRange((void*)addr, 4);                                          \
        ICInvalidateRange((void*)addr, 4);                                     \
    } while (0)

u8 Stage3Key[] = {
    0x6C, 0xF9, 0xF3, 0x92, 0x2C, 0x31, 0xAC, 0x7A, 0x65, 0x12, 0x37, 0x07,
    0x47, 0x06, 0x01, 0x53, 0x0C, 0x2D, 0x24, 0x78, 0xBF, 0xE6, 0xEE, 0x0A,
    0xE7, 0x0B, 0xD9, 0x57, 0x10, 0x9C, 0x5B, 0xD4, 0x1A, 0x83, 0xF5, 0xB6,
    0x4F, 0x87, 0x45, 0x84, 0x7C, 0xD1, 0x64, 0x1F, 0xDD, 0x7E, 0xA2, 0x80,
    0xDE, 0x7D, 0x8F, 0x77, 0x5C, 0xB2, 0xCE, 0xF7, 0xC2, 0x82, 0xBB, 0x8E,
    0x03, 0x11, 0x89, 0x7B, 0xBC, 0xB5, 0x8D, 0x14, 0x56, 0x30, 0x15, 0x93,
    0x05, 0x20, 0xA8, 0xAE, 0xD7, 0xB3, 0xD2, 0x97, 0xDF, 0x0D, 0xEF, 0x66,
    0x85, 0x2F, 0x1E, 0xB9, 0xCA, 0xC3, 0x67, 0xA7, 0xBD, 0x27, 0xD5, 0x2A,
    0x79, 0xB8, 0x98, 0x9D, 0x96, 0x6F, 0x25, 0x74, 0x0E, 0x3E, 0x16, 0xD3,
    0xA5, 0x19, 0xDC, 0x50, 0x49, 0x08, 0x4C, 0xB0, 0x2E, 0xFC, 0xC7, 0xE3,
    0xCC, 0xFE, 0x02, 0x42, 0xC6, 0xBA, 0x9A, 0x1C, 0xB7, 0xBE, 0x43, 0xCB,
    0x3A, 0x73, 0x09, 0x21, 0x5A, 0xAB, 0x13, 0x54, 0xF4, 0xCF, 0x33, 0xC4,
    0x17, 0xC5, 0xCD, 0x32, 0x63, 0x4A, 0xC1, 0x4B, 0x23, 0x41, 0x8B, 0x3D,
    0x7F, 0xA1, 0xF0, 0xDB, 0xED, 0xFD, 0xC0, 0x8A, 0x61, 0x5F, 0x34, 0x3C,
    0xE4, 0x55, 0x38, 0x9B, 0x36, 0x60, 0x99, 0x18, 0xD6, 0xEC, 0xE5, 0x46,
    0x69, 0x68, 0x6B, 0x3F, 0x81, 0x29, 0xC9, 0xEB, 0x59, 0xE0, 0xE1, 0xF2,
    0xDA, 0xE2, 0x51, 0x5E, 0xC8, 0x44, 0xF6, 0x9F, 0x90, 0xB4, 0x72, 0x6E,
    0x1D, 0x9E, 0x58, 0x86, 0x75, 0xE9, 0x0F, 0x70, 0x1B, 0x39, 0xFF, 0xE8,
    0x6D, 0xA3, 0x71, 0x3B, 0xAA, 0x26, 0xA6, 0xAD, 0x8C, 0x91, 0xA9, 0x00,
    0x5D, 0xF1, 0x62, 0xF8, 0xD0, 0x52, 0x6A, 0x28, 0xFB, 0xFA, 0xD8, 0x22,
    0x95, 0x04, 0x4D, 0xA0, 0x2B, 0x40, 0xB1, 0x94, 0x35, 0xA4, 0xAF, 0x4E,
    0xEA, 0x88, 0x48, 0x76,
};

typedef struct
{
    union
    {
        struct
        {
            u32 dol_text[7];
            u32 dol_data[11];
        };
        u32 dol_sect[7 + 11];
    };
    union
    {
        struct
        {
            u32 dol_text_addr[7];
            u32 dol_data_addr[11];
        };
        u32 dol_sect_addr[7 + 11];
    };
    union
    {
        struct
        {
            u32 dol_text_size[7];
            u32 dol_data_size[11];
        };
        u32 dol_sect_size[7 + 11];
    };
    u32 dol_bss_addr;
    u32 dol_bss_size;
    u32 dol_entry_point;
    u32 dol_pad[0x1C / 4];
} DOL;

static inline void clearWords(u32* data, u32 count)
{
    while (count--)
    {
        asm volatile("dcbz    0, %0\n"
                     //"sync\n"
                     "dcbf    0, %0\n" ::"r"(data));
        data += 8;
    }
}

static inline void copyWords(u32* dest, u32* src, u32 count)
{
    register u32 value = 0;
    while (count--)
    {
        asm volatile("dcbz    0, %1\n"
                     //"sync\n"
                     "lwz     %0, 0(%2)\n"
                     "stw     %0, 0(%1)\n"
                     "lwz     %0, 4(%2)\n"
                     "stw     %0, 4(%1)\n"
                     "lwz     %0, 8(%2)\n"
                     "stw     %0, 8(%1)\n"
                     "lwz     %0, 12(%2)\n"
                     "stw     %0, 12(%1)\n"
                     "lwz     %0, 16(%2)\n"
                     "stw     %0, 16(%1)\n"
                     "lwz     %0, 20(%2)\n"
                     "stw     %0, 20(%1)\n"
                     "lwz     %0, 24(%2)\n"
                     "stw     %0, 24(%1)\n"
                     "lwz     %0, 28(%2)\n"
                     "stw     %0, 28(%1)\n"
                     "dcbf    0, %1\n" ::"r"(value),
                     "r"(dest), "r"(src));
        dest += 8;
        src += 8;
    }
}

u32 LoadDOL(DOL* dol)
{
    clearWords((u32*)dol->dol_bss_addr, dol->dol_bss_size / 32);

    for (int i = 0; i < 7 + 11; i++)
    {
        if (dol->dol_sect_size[i] != 0)
        {
            copyWords((u32*)dol->dol_sect_addr[i],
                      (u32*)((u32)dol + dol->dol_sect[i]),
                      dol->dol_sect_size[i] / 32);
        }
    }

    return dol->dol_entry_point;
}

// see patch.S
extern u32 PatchCodePStart[];
extern u32 PatchCodePEnd[];

extern u32 PatchCodeEStart[];
extern u32 PatchCodeEEnd[];

extern u32 PatchCodeJStart[];
extern u32 PatchCodeJEnd[];

extern u32 PatchCodeKStart[];
extern u32 PatchCodeKEnd[];

void GameEntryHookWrapper();
asm(".global GameEntryHookWrapper\n"
    "GameEntryHookWrapper:\n"
    "   mr 30, 9\n"
    "   mr 3, 9\n"
    "   bl GameEntryHook\n"
    // "   bl __InitBATS\n"
    "   mtctr 30\n"
    "   bctr\n");

void GameEntryHook(u32 entry_point)
{

    for (u32 addr = 0x81700000; addr < 0x81800000; addr += 4)
    {
        if (*(u32*)addr == 0x7D3C8BA6 && *(u32*)(addr - 8) == 0x7C7D1B78)
        {
            POKE32(addr - 0x1C, 0x4E800020);
            break;
        }
    }

    POKE32(0x817DDCC0, 0x4E800020); // disable codehandler search

    // search for the end of bad1 using the following strategy: these
    // instructions that should never appear next to each other yet they do
    // because chadsoft code.
    u32 bad1PatchAddr = 0;
    for (u32 addr = 0x816E0000; addr < 0x81700000; addr += 4)
    {
        if (*(u32*)addr == 0x3AE00000 && *(u32*)(addr + 4) == 0x4E800020 &&
            *(u32*)(addr + 8) == 0)
        {
            extern void BAD1_P_Patch(void* dest, const void* src, u32 len);
            extern void BAD1_P_PatchEnd();

            bad1PatchAddr = addr + 8;

            // lol
            BAD1_P_Patch((void*)bad1PatchAddr, &BAD1_P_Patch,
                         (u32)&BAD1_P_PatchEnd - (u32)&BAD1_P_Patch);
            break;
        }
    }

    while (bad1PatchAddr == 0)
    {
    }

    // search for these fun instructions
    for (u32 addr = 0x816D4000; addr < 0x817FF000; addr += 4)
    {
        if (*(u32*)addr == 0x3BBE0010 && *(u32*)(addr + 4) == 0x847D0010 &&
            *(u32*)(addr + 0x20) == 0x4E800021)
        {
            POKE_BL(addr + 0x20, bad1PatchAddr);
        }
    }

#if 0
    switch (*(char*)0x80000003)
    {
    case 'P':
        (*(void (*)(void))0x801A7CAC)();
        break;

    case 'E':
        (*(void (*)(void))0x801A7C0C)();
        break;

    case 'J':
        (*(void (*)(void))0x801A7BCC)();
        break;

    case 'K':
        (*(void (*)(void))0x801A8008)();
        break;
    }
#endif
    DCFlushRange((void*)entry_point, 0x10000);
    ICInvalidateRange((void*)entry_point, 0x10000);
}

void Stage3KeyHook();
asm(".global Stage3KeyHook\n"
    "Stage3KeyHook:\n"
    "   mtctr 10\n"
    "   lis 31, Stage3Key@h\n"
    "   ori 31, 31, Stage3Key@l\n"
    "   blr\n");

void Stage4EntryHook()
{
    // POKE32(0x8004377C, 0x48000785);
    // POKE32(0x8004389C, 0x48000685);
    // POKE32(0x8004394C, 0x480005D5);
    // POKE32(0x80043AC0, 0x48000481);
    POKE32(0x80043C7C, 0x7D4903A6);
    POKE32(0x80043EA4, 0x487BC15D);

    bool foundPatch1 = false;
    bool foundPatch2 = false;
    bool foundPatch3 = false;
    bool foundPatch4 = false;

    for (u32 addr = 0x80840000; addr < 0x80970000; addr += 4)
    {
        if (foundPatch1 && foundPatch2 && foundPatch3 && foundPatch4)
            break;

        if (!foundPatch1)
        {
            if (*(u32*)addr == 0x3D4A0100 && *(u32*)(addr + 4) == 0x91490000)
            {
                POKE32(addr + 4, 0x60000000);
                foundPatch1 = true;
                continue;
            }
        }

        if (!foundPatch2)
        {
            if (*(u16*)(addr) == 0x812D && *(u32*)(addr + 4) == 0x7D2903A6 &&
                *(u32*)(addr + 8) == 0x4E800421)
            {
                POKE_BL(addr + 8, GameEntryHookWrapper);
                foundPatch2 = true;
                continue;
            }
        }

        if (!foundPatch3)
        {
            // patch to __pad_onreset to hacky fix an issue causing blackscreen
            // on game launch
            // this branch is static enough to include in the search
            if (*(u32*)(addr) == 0x419E000C &&
                *(u32*)(addr + 4) == 0x38600000 &&
                *(u32*)(addr + 8) == 0x4BFFFF9D)
            {
                POKE32(addr + 0x28, 0x41BE0018);
                foundPatch3 = true;
                continue;
            }
        }

        if (!foundPatch4)
        {
            if (*(u32*)(addr) == 0x7D3F83A6 && *(u32*)(addr + 4) == 0x3FA08080)
            {
                u32 patchAddr = addr - 0x28;
                POKE_B(patchAddr, patchAddr + 0x284);
            }
        }
    }

    // void __InitBATS();
    // POKE_BL(0x809317A0, __InitBATS);
    // POKE_B(0x8093170C, 0x80931990); // skip page tables in channel
    // POKE_B(0x80951744, 0x8095175C); // skip cfi setup for dol

    (*(void (*)(void))0x80800000)();
}

void Stage3EntryHook(u32 addr)
{
    POKE32(0x813000B0, 0x4E800420);

    // POKE32(0x8004377C, 0x38600018);
    // POKE32(0x8004389C, 0x386000E1);
    // POKE32(0x8004394C, 0x386000BC);
    // POKE32(0x80043AC0, 0x386000BC);

    POKE_BL(0x80043C7C, Stage3KeyHook);
    POKE_BL(0x80043EA4, Stage4EntryHook);

    (*(void (*)(u32, u32))0x80048350)(0, 0);
}

void Stage2LoaderEntryHook(u32 dolAddr)
{
    POKE32(0x80783DE8, 0x4E800421);

    POKE_B((void*)0x813000B0, Stage3EntryHook);

    (*(void (*)(u32))0x81300000)(dolAddr);
}

void Stage2EntryHook()
{
    POKE32(0x80007D50, 0x4E800421);

    POKE_BL(0x80783DE8, Stage2LoaderEntryHook);

    (*(void (*)(void))0x80780000)();
}

void load()
{
    extern u8 sd_stub_v1_dol[];

    u32 entry_point = LoadDOL((DOL*)sd_stub_v1_dol);

    POKE_BL(0x80007D50, Stage2EntryHook);

    (*(void (*)(void))entry_point)();
}
