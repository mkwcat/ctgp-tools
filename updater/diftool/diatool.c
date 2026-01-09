#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/*+--------------------------------------------------------------------------+*/
typedef uint8_t u8; ///< 8bit unsigned integer
typedef uint16_t u16; ///< 16bit unsigned integer
typedef uint32_t u32; ///< 32bit unsigned integer
typedef uint64_t u64; ///< 64bit unsigned integer
/*+--------------------------------------------------------------------------+*/
typedef int8_t s8; ///< 8bit signed integer
typedef int16_t s16; ///< 16bit signed integer
typedef int32_t s32; ///< 32bit signed integer
typedef int64_t s64; ///< 64bit signed integer
/*+--------------------------------------------------------------------------+*/
typedef volatile u8 vu8; ///< 8bit unsigned volatile integer
typedef volatile u16 vu16; ///< 16bit unsigned volatile integer
typedef volatile u32 vu32; ///< 32bit unsigned volatile integer
typedef volatile u64 vu64; ///< 64bit unsigned volatile integer
/*+--------------------------------------------------------------------------+*/
typedef volatile s8 vs8; ///< 8bit signed volatile integer
typedef volatile s16 vs16; ///< 16bit signed volatile integer
typedef volatile s32 vs32; ///< 32bit signed volatile integer
typedef volatile s64 vs64; ///< 64bit signed volatile integer
/*+--------------------------------------------------------------------------+*/

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

typedef struct _dif_source
{
    char package[8];
    u32 name_offset;
    u32 flags; /* 1<<0 = Source is Yaz0 compressed */
} DIF_Source;

typedef struct _dif_header
{
    char magic[4]; /* Always "DIF0" */
    u32 dif_size;
    u32 unknown_c;
    u32 name_offset;
    u32 header_size;
    u32 flags; /* 1<<0 = Target is Yaz0 compressed */
    u32 unknown_18;
    u32 source_count;
} DIF_Header;

FILE* dia;
DIA_Header hdr;
DIA_Diftab_Hdr dhdr;
DIA_Diftab_Entry* dent;

u16 bswap16(u16 v)
{
    return ((v & 0x00FF) << 8) | ((v & 0xFF00) >> 8);
}

u32 bswap32(u32 v)
{
    return ((v & 0x000000FF) << 24) | ((v & 0x0000FF00) << 8) |
           ((v & 0x00FF0000) >> 8) | ((v & 0xFF000000) >> 24);
}

void diatool_exit()
{
    fclose(dia);
    exit(EXIT_FAILURE);
}

void DIA_ReadDIFTable()
{
    fseek(dia, bswap32(hdr.diftab_offset), SEEK_SET);
    if (fread(&dhdr, sizeof(DIA_Diftab_Hdr), 1, dia) != 1)
    {
        printf("diatool: cannot read DIF table header\n");
        diatool_exit();
    }
    dent = malloc(sizeof(DIA_Diftab_Entry) * bswap32(hdr.diftab_count));
    if (fread(dent, sizeof(DIA_Diftab_Entry) * bswap32(hdr.diftab_count), 1,
              dia) != 1)
    {
        printf("diatool: cannot read DIF table\n");
        diatool_exit();
    }
}

void DIA_ReadHeader()
{
    if (fread(&hdr, sizeof(DIA_Header), 1, dia) != 1)
    {
        printf("diatool: cannot read file header\n");
        diatool_exit();
    }
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf("%s <file.dif>\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (!(dia = fopen(argv[1], "rb")))
    {
        printf("diatool: cannot open file\n");
        return EXIT_FAILURE;
    }

    DIA_ReadHeader();
    DIA_ReadDIFTable();

    printf("Package: %.8s\n", &hdr.package);
    printf("Version: %X.%02X.%04X (from %.8s %X.%02X.%04X)\n",
           hdr.version.major, hdr.version.middle, bswap16(hdr.version.minor),
           dhdr.former_package, dhdr.former_version.major,
           dhdr.former_version.middle, bswap16(dhdr.former_version.minor));

    DIF_Header difhdr;
    char file_name[200];
    for (int i = 0; i < bswap32(hdr.diftab_count); i++)
    {
        fseek(dia, bswap32(dent[i].offset), SEEK_SET);
        if (fread(&difhdr, 0x20, 1, dia) != 1)
        {
            printf("DIF0 fail\n");
            continue;
        }
        fseek(dia, bswap32(dent[i].offset) + bswap32(difhdr.name_offset),
              SEEK_SET);
        int name_size = bswap32(dent[i].size) - bswap32(difhdr.name_offset);
        if (fread(file_name, name_size <= 200 ? name_size : 200, 1, dia) != 1)
        {
            printf("DIF0 fail\n");
            continue;
        }
        printf("DIF0: %s\n", file_name);
    }

    fclose(dia);
    return EXIT_SUCCESS;
}
