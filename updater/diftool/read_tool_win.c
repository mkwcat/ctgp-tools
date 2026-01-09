#include "dif_archive.h"
#include "gctypes.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winbase.h>

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
    printf("Version: %X.%02X.%04X (from %X.%02X.%04X)\n", hdr.version.major,
           hdr.version.middle, bswap16(hdr.version.minor),
           dhdr.former_version.major, dhdr.former_version.middle,
           bswap16(dhdr.former_version.minor));



    char realFileName[256];

    snprintf(realFileName, 256, "D:\\ctgp\\update\\archive_real\\%.8s%02X%02X%04X-%.8s%02X%02X%04X.dif",
           &dhdr.former_package, dhdr.former_version.major,
           dhdr.former_version.middle, bswap16(dhdr.former_version.minor),
           &hdr.package, hdr.version.major, hdr.version.middle,
           bswap16(hdr.version.minor));

    BOOL rersult = CopyFile(argv[1], realFileName, TRUE);
    if (rersult == 0) {
        printf("This is already in the archive!\n");
    } else {
        printf("You found something not in the archive!!!! Congrats\n");
    }

    DIF_Header difhdr;
    char file_name[200];
    for (int i = 0; i < bswap32(hdr.diftab_count); i++)
    {
        fseek(dia, bswap32(dent[i].offset), SEEK_SET);
        fread(&difhdr, 0x20, 1, dia);
        fseek(dia, bswap32(dent[i].offset) + bswap32(difhdr.name_offset),
              SEEK_SET);
        int name_size = bswap32(dent[i].size) - bswap32(difhdr.name_offset);
        fread(file_name, name_size <= 200 ? name_size : 200, 1, dia);
        printf("DIF0: %s\n", file_name);
    }

    fclose(dia);
    return EXIT_SUCCESS;
}
