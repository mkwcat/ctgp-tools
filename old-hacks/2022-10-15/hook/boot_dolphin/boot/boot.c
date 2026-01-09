#include <gctypes.h>
#include <ogc/cache.h>

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

void GameEntryHookWrapper();
asm(".global GameEntryHookWrapper\n"
    "GameEntryHookWrapper:\n"
    "   mr 3, 9\n"
    "   b GameEntryHook\n");

void GameEntryHook(u32 entryPoint)
{
#if 0
    for (u32 addr = 0x81700000; addr < 0x81800000; addr += 4)
    {
        if (*(u32*)addr == 0x7D3C8BA6 && *(u32*)(addr - 8) == 0x7C7D1B78)
        {
            POKE32(addr - 0x1C, 0x4E800020);
            break;
        }
    }
#endif

    (*(void (*)(void))entryPoint)();
}

void Stage4EntryHook()
{
    POKE32(0x8004377C, 0x48000785);
    POKE32(0x8004389C, 0x48000685);
    POKE32(0x8004394C, 0x480005D5);
    POKE32(0x80043AC0, 0x48000481);
    POKE32(0x80043EA4, 0x487BC15D);

    bool foundPatch1 = false;
    bool foundPatch2 = false;

    // Search for anti-dolphin cache trick
    for (u32 addr = 0x80840000; addr < 0x80940000; addr += 4)
    {
        if (!foundPatch1)
        {
            if (*(u32*)addr == 0x3D4A0100 && *(u32*)(addr + 4) == 0x91490000)
            {
                POKE32(addr + 4, 0x60000000);
                if (foundPatch2)
                    break;
                continue;
            }
        }

        if (!foundPatch2)
        {
            if (*(u16*)(addr) == 0x812D && *(u32*)(addr + 4) == 0x7D2903A6 &&
                *(u32*)(addr + 8) == 0x4E800421)
            {
                POKE_BL(addr + 8, GameEntryHookWrapper);
                if (foundPatch1)
                    break;
                continue;
            }
        }
    }

    (*(void (*)(void))0x80800000)();
}

void Stage3EntryHook(u32 addr)
{
    POKE32(0x813000B0, 0x4E800420);

    POKE32(0x8004377C, 0x38600018);
    POKE32(0x8004389C, 0x386000E1);
    POKE32(0x8004394C, 0x386000BC);
    POKE32(0x80043AC0, 0x386000BC);

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
