#pragma once

#include "gctypes.h"

#define DIA_MAGIC "DIA0"
#define DIA_MAGIC0 'D'
#define DIA_MAGIC1 'I'
#define DIA_MAGIC2 'A'
#define DIA_MAGIC3 '0'
#define DIA_HEADER_SIZE 0x64

#define DIF_MAGIC "DIF0"
#define DIF_MAGIC0 'D'
#define DIF_MAGIC1 'I'
#define DIF_MAGIC2 'F'
#define DIF_MAGIC3 '0'
#define DIF_HEADER_SIZE 0x20

typedef struct _dia_version
{
    u8 major;
    u8 middle;
    u16 minor;
} DIA_Version;

typedef struct _dia_header
{
    char magic[4]; /* Always "DIA0". */
    u32 dia_size;
    u32 unknown_8;
    u32 unknown_c;
    char package[8];
    DIA_Version version;
    u32 unknown_1c;
    u32 unknown_20;
    u8 unknown_24[0x2E]; /* Probably a hash of some sort. */
    u16 unknown_52;
    u32 unknown_54;
    u32 diftab_offset;
    u32 diftab_count;
    u32 unknown_60;
} DIA_Header;

typedef struct _dia_diftab_hdr
{
    char former_package[8];
    DIA_Version former_version;
    u32 unknown_c;
} DIA_Diftab_Hdr;

typedef struct _dia_diftab_entry
{
    u32 offset;
    u32 size;
} DIA_Diftab_Entry;

typedef struct _dia_dif
{
    char magic[4]; /* Always "DIF0" */
    u32 dif_size;
    u32 unknown_c;
    u32 name_offset;
    u32 header_size;
    u8 unknown_14;
    u8 unknown_15;
    u8 unknown_16;
    u8 unknown_17; /* Probably to do with Yaz0 compression. */
    u32 unknown_18;
    u32 unknown_1c;
    /* Values of expanded header */
    char package[8]; /* Not sure what this really is. */
    u32 name_offset_2;
    u32 unknown_2c;
} DIF_Header;
