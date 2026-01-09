#include "dif.h"
#include <sys/stat.h>
#include <string.h>

u8* dia_data;
int dia_size;

FILE* outfile;

int main(int argc, char** argv)
{
    char* pack_name;
    char* rmc_name = (char*)"RMC";
    char* dia_name = (char*)"magic.dif";

    int pack_set = 0;
    int rmc_set = 0;
    int dia_set = 0;
    int skip_count = 0;

    if (argc % 2 == 0)
    {
        printf("usage: %s [--pack <pack>] [--rmc <rmc>] [--dia <dia>]\n",
               argv[0]);
        return 1;
    }

    int i = 1;
    while (i < argc)
    {
        if (!strncmp(argv[i], "--pack", 7) && !pack_set)
        {
            pack_name = argv[i + 1];
            pack_set = 1;
        }
        else if (!strncmp(argv[i], "--rmc", 6) && !rmc_set)
        {
            rmc_name = argv[i + 1];
            rmc_set = 1;
        }
        else if (!strncmp(argv[i], "--dia", 6) && !dia_set)
        {
            dia_name = argv[i + 1];
            dia_set = 1;
        }
        else if (!strncmp(argv[i], "--skip", 7) && !skip_count)
        {
            sscanf(argv[i + 1], "%u", &skip_count);
        }
        else
        {
            printf("usage: %s [--pack <pack>] [--rmc <rmc>] [--dia <dia>]\n",
                   argv[0]);
            return 1;
        }

        i += 2;
    }

    FILE* dia_file = fopen(dia_name, "rb");
    if (!dia_file)
    {
        printf("failed to open DIA file (%s)!!\n", dia_name);
        return 1;
    }

    fseek(dia_file, 0, SEEK_END);
    dia_size = ftell(dia_file);
    dia_data = (u8*)malloc(dia_size);
    assert(dia_data);
    fseek(dia_file, 0, SEEK_SET);
    fread(dia_data, 1, dia_size, dia_file);
    fclose(dia_file);

    yaz = (Yaz_file_struct*)malloc(sizeof(*yaz));

    if (!pack_set)
        pack_name = (char*)&((DIA_Header*)dia_data)->package;

    DIA_Diftab_Entry* entries = (DIA_Diftab_Entry*)&dia_data[bswap32(
        ((DIA_Header*)dia_data)->diftab_entry_offset)];
    u32 entry_count = bswap32(((DIA_Header*)dia_data)->diftab_entry_count);

    for (u32 i = skip_count; i < entry_count; ++i)
    {
        u32 infilesize = bswap32(entries[i].size);
        u8* indata = (u8*)malloc(infilesize);
        memcpy((void*)indata, &dia_data[bswap32(entries[i].offset)],
               infilesize);

        printf("updating file #%u (%s):\n", i + 1,
               &indata[bswap32(((DIF_Header*)indata)->name_offset)]);

        if (int err = OpenExpFiles(pack_name, rmc_name, indata, infilesize))
        {
            printf("  failed to open expanded file %X!!\n", err);
            CloseExpFiles();
            free(indata);
            continue;
        }

        mkdir(pack_name, 07777);

        char out_name[256];
        snprintf(
            out_name, 256, "./%.8s%s", pack_name,
            (const char*)&indata[bswap32(((DIF_Header*)indata)->name_offset)]);

        if (out_name[strlen(out_name) - 1] == '/')
        {
            out_name[strlen(out_name) - 1] = 0;
            printf("  creating directory (%s)\n", out_name);
            mkdir(out_name, 07777);
            free(indata);
        }
        else
        {
            outfile = fopen(out_name, "wb");
            if (!outfile)
            {
                printf("  failed to open output file (%s)!!\n", out_name);
                CloseExpFiles();
                free(indata);
                continue;
            }
            printf("  opened output file!\n");

            ProcessDif(indata, outfile, yaz);
            CloseExpFiles();
            free(indata);
            fclose(outfile);

            printf("  done!\n");
        }
    }

    free(dia_data);
    free(yaz);

    return 0;
}
