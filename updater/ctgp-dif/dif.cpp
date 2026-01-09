#include "dif.h"
#include "types.h"
#include "yaz.h"
#include <cstring>

#define MAXFILES 15

static u8* expdata[MAXFILES];
static u32 expfilesize[MAXFILES];
static u32 expcursors[MAXFILES + 1];

u32 bswap32(u32 v)
{
    return (v << 24) | ((v << 8) & 0xFF0000) | ((v >> 8) & 0xFF00) | (v >> 24);
}

int OpenExpFiles(const char* pack, const char* rmc, u8* data, u32 size)
{
    memset(expdata, 0, sizeof(expdata));

    u32 expFiles = bswap32(((DIF_Header*)data)->expansion_count);

    int i = 0;
    while (i < expFiles && i < MAXFILES)
    {
        expcursors[i + 1] = 0;
        char filename[256];

        DIF_Header* head = (DIF_Header*)data;
        const char* prefix;

        if (memcmp(head->exp[i].package, "RMC\0\0\0\0\0", 8) == 0)
            prefix = rmc;
        else
            prefix = pack;

        snprintf(
            filename, 256, "./%s%s", prefix,
            (char*)(data + bswap32(((DIF_Header*)data)->exp[i].name_offset)));

        FILE* expfile = fopen(filename, "rb");
        if (!expfile)
            return i + 1;

        fseek(expfile, 0, SEEK_END);
        expfilesize[i] = ftell(expfile);
        expdata[i] = (u8*)malloc(expfilesize[i]);
        assert(expdata[i]);
        fseek(expfile, 0, SEEK_SET);
        fread(expdata[i], 1, expfilesize[i], expfile);
        fclose(expfile);

        if (head->exp[i].type == TYPE_YAZ0)
        {
            u8* compData = expdata[i];

            u32 expandSize = bswap32(*(u32*)&compData[4]);
            printf("expandSize: %08X\n", expandSize);

            u8* decompData = (u8*)malloc(expandSize);
            assert(decompData);
            decodeYaz0(compData + 0x10, expfilesize[i], decompData, expandSize);
            free(expdata[i]);

            expdata[i] = decompData;
            expfilesize[i] = expandSize;
        }

        printf("  finished reading expanded file %X (%s)!\n", ++i, filename);
    }

    return 0;
}

void CloseExpFiles()
{
    for (int i = 0; i < MAXFILES; i++)
    {
        if (expdata[i] != nullptr)
            free(expdata[i]);
    }
}

void ProcessDif(u8* data, u32 size, FILE* file)
{
    DIF_Header* head = (DIF_Header*)data;

    u32 dataOffset = bswap32(head->header_size);
    u32 inputDataSize = bswap32(head->name_offset);

    Yaz_file_struct yaz;
    if (head->type == TYPE_YAZ0)
    {
        bool ret = Yaz_open(&yaz, file);
        assert(ret);
    }

    std::vector<u8> outdata;

    u32 i = dataOffset;
    ;
    while (i < inputDataSize)
    {
        u32 oldlen = outdata.size();

        u8 b = data[i++];
        u8 c = b & 15;
        int y = 0;
        u32 j = 0;
        u8 z = 0;

        if (b == 0xF1)
            break;

        switch (c)
        {
        case 0: {
            y = b >> 4;
            outdata.insert(outdata.end(), data + i, data + i + y + 1);
            i += y + 1;
            expcursors[0] = outdata.size() - 1;
            break;
        }
        case 1: {
            if (b >> 4 == 15)
                break;
            y = data[i++];
            u8 s = b >> 4;
            while (s)
            {
                y = (y + 1) * 256 + data[i++];
                --s;
            }
            outdata.insert(outdata.end(), data + i, data + i + y + 17);
            i += y + 17;
            expcursors[0] = outdata.size() - 1;
            break;
        }
        case 3: {
            y = b >> 4;
            j = expcursors[0];
            while (y > -2)
            {
                outdata.insert(outdata.end(), &outdata[j], &outdata[j + 1]);
                ++j;
                --y;
            }
            expcursors[0] = outdata.size() - 1;
            break;
        }
        case 4: {
            z = b >> 4;
            y = data[i++];
            j = expcursors[z];
            if (z)
            {
                expcursors[z] += y + 6;
                --z;
                outdata.insert(outdata.end(), expdata[z] + j,
                               expdata[z] + j + y + 6);
            }
            else
                while (y > -6)
                {
                    outdata.insert(outdata.end(), &outdata[j], &outdata[j + 1]);
                    ++j;
                    --y;
                }
            expcursors[0] = outdata.size() - 1;
            break;
        }
        case 5: {
            z = b >> 4;
            y = (data[i] + 1) * 256 + data[i + 1];
            i += 2;
            j = expcursors[z];
            if (z)
            {
                expcursors[z--] += y + 6;
                outdata.insert(outdata.end(), expdata[z] + j,
                               expdata[z] + j + y + 6);
            }
            else
                while (y > -6)
                {
                    outdata.insert(outdata.end(), &outdata[j], &outdata[j + 1]);
                    ++j;
                    --y;
                }
            expcursors[0] = outdata.size() - 1;
            break;
        }
        case 6: {
            z = b >> 4;
            y = (data[i] + 1) * 65536 + (data[i + 1] + 1) * 256 + data[i + 2];
            i += 3;
            j = expcursors[z];
            if (z)
            {
                expcursors[z--] += y + 6;
                outdata.insert(outdata.end(), expdata[z] + j,
                               expdata[z] + j + y + 6);
            }
            else
                while (y > -6)
                {
                    outdata.insert(outdata.end(), &outdata[j], &outdata[j + 1]);
                    ++j;
                    --y;
                }
            expcursors[0] = outdata.size() - 1;
            break;
        }
        case 7: {
            z = b >> 4;
            y = (data[i] + 1) * 1677216 + (data[i + 1] + 1) * 65536 +
                (data[i + 2] + 1) * 256 + data[i + 3];
            i += 4;
            j = expcursors[z];
            if (z)
            {
                expcursors[z--] += y + 6;
                outdata.insert(outdata.end(), expdata[z] + j,
                               expdata[z] + j + y + 6);
            }
            else
                while (y > -6)
                {
                    outdata.insert(outdata.end(), &outdata[j], &outdata[j + 1]);
                    ++j;
                    --y;
                }
            expcursors[0] = outdata.size() - 1;
            break;
        }
        case 8: {
            y = b >> 4;
            expcursors[0] -= (y + 1);
            break;
        }
        case 9: {
            z = b >> 4;
            y = data[i++];
            expcursors[z] -= (y + 1);
            break;
        }
        case 10: {
            z = b >> 4;
            y = (data[i] + 1) * 256 + data[i + 1];
            i += 2;
            expcursors[z] -= (y + 1);
            break;
        }
        case 11: {
            z = b >> 4;
            y = (data[i] + 1) * 65536 + (data[i + 1] + 1) * 256 + data[i + 2];
            i += 3;
            expcursors[z] -= (y + 1);
            break;
        }
        case 12: {
            z = b >> 4;
            y = (data[i] + 1) * 16777216 + (data[i + 1] + 1) * 65536 +
                (data[i + 2] + 1) * 256 + data[i + 3];
            i += 4;
            expcursors[z] -= (y + 1);
            break;
        }
        case 13: {
            z = b >> 4;
            expcursors[z] += data[i++] + 1;
            break;
        }
        case 14: {
            z = b >> 4;
            expcursors[z] += (data[i] + 1) * 256 + data[i + 1] + 1;
            i += 2;
            break;
        }
        case 15: {
            z = b >> 4;
            expcursors[z] = data[i] * 65536 + data[i + 1] * 256 + data[i + 2];
            i += 3;
            break;
        }
        default: {
            printf("  unknown command 2 at 0x%X!\n", i - 1);
            return;
        }
        }

        if (head->type == TYPE_YAZ0)
        {
            Yaz_write(&yaz, outdata.data() + oldlen, outdata.size() - oldlen);
        }
        else
        {
            fwrite(outdata.data() + oldlen, outdata.size() - oldlen, 1, file);
        }
    }

    if (head->type == TYPE_YAZ0)
    {
        Yaz_write(&yaz, nullptr, 0);

        fseek(yaz.file, 4, SEEK_SET);

        u8 decompSize[4];
        decompSize[0] = yaz.iteration >> 24;
        decompSize[1] = yaz.iteration >> 16;
        decompSize[2] = yaz.iteration >> 8;
        decompSize[3] = yaz.iteration;

        fwrite(decompSize, 1, 4, yaz.file);
    }
}

/*int dif_main(int argc, char **argv)
{
    if (argc < 5)
    {
        printf("  usage: %s <in dif0> <out bin> <package> <rmc>\n", argv[0]);
        return 1;
    }

    const char* infilename = argv[1];
    const char* outfilename = argv[2];

    FILE* infile = fopen(infilename, "rb");
    if (!infile)
        return 1;

    fseek(infile, 0, SEEK_END);
    infilesize = ftell(infile);
    indata = (u8*)malloc(infilesize);
    assert(indata);
    fseek(infile, 0, SEEK_SET);
    fread(indata, 1, infilesize, infile);
    fclose(infile);

    printf("  finished reading input file!\n");

    if (OpenExpFiles(argv[3], argv[4], indata, infilesize))
        return 1;

    outfile = fopen(outfilename, "wb");
    assert(outfile);

    printf("  opened output file!\n");

    ProcessDif(indata, infilesize);
    fclose(outfile);

    printf("  done!\n");
    return 0;
}*/
