#pragma once

#include "types.h"

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
} DIA_Version; /* "%1.1hhX.%2.2hhX.%4.4hX", major, middle, minor */

typedef struct _dia_header
{
    char magic[4]; /* always "DIA0". */
    u32 dia_size;
    u32 unknown_8; /* always 0. */
    u32 unknown_c; /* always 0. */
    char package[8];
    DIA_Version version;
    u32 total_decomp_size;
    u32 sign_size;
    u8 sign[0x30]; /* excess space at the end is filled with zeroes. */
    u32 fomver_ver_count;
    u32 former_ver_offset;
    u32 diftab_entry_count;
    u32 diftab_entry_offset;
} DIA_Header;

typedef struct _dia_former_ver
{
    char package[8]; /* sometimes "RMC", meaning this update doesn't rely on a
                        previous version. */
    DIA_Version former_version; /* RMC is always 1.00.0000. */
    u32 unknown_c; /* always 0. */
} DIA_Former_Version;

typedef struct _dia_diftab_entry
{
    u32 offset;
    u32 size;
} DIA_Diftab_Entry;

typedef enum _dif_filetype : u8
{
    TYPE_NORMAL = 0,
    TYPE_YAZ0 = 1,
    TYPE_DIRECTORY = 4
} FileType;

typedef struct _dia_dif_exp
{
    char package[8];
    u32 name_offset;
    u8 unknown_c; /* always 0. */
    u8 unknown_d; /* always 0. */
    u8 unknown_e; /* always 0. */
    FileType type; /* never DIRECTORY. */
} DIF_Expansion;

typedef struct _dia_dif
{
    char magic[4]; /* always "DIF0". */
    u32 dif_size;
    u32 unknown_8; /* always 0. */
    u32 name_offset; /* 0x20 for directories. */
    u32 header_size; /* 0 for directories. */
    u8 unknown_14; /* always 0. */
    u8 exists; /* 02: a file with the same name already exists. doesn't apply to
                  SD updates. */
    u8 unknown_16; /* always 0. */
    FileType type;
    u32 decomp_size; /* 0 for directories. */
    u32 expansion_count; /* 0 for directories. */
    DIF_Expansion exp[0];
} DIF_Header;
