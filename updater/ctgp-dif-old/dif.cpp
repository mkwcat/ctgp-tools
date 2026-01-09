#include "dif.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

u32 bswap32(u32 v)
{
    return (v << 24) | ((v << 8) & 0xFF0000) | ((v >> 8) & 0xFF00) | (v >> 24);
}

int OpenExpFiles(const char *pack, const char *rmc, u8 *data, u32 size)
{
    u32 expFiles = bswap32(((DIF_Header*)data)->expansion_count);
    
    int i = 0;
    while (i < expFiles && i < MAXFILES)
    {
        expcursors[i + 1] = 0;
        char *filename = (char*)malloc(65536);
        if (((DIF_Header*)data)->exp[i].package[0] == 'R'
                && ((DIF_Header*)data)->exp[i].package[1] == 'M'
                && ((DIF_Header*)data)->exp[i].package[2] == 'C'
                && !(((DIF_Header*)data)->exp[i].package[3]
                || ((DIF_Header*)data)->exp[i].package[4]
                || ((DIF_Header*)data)->exp[i].package[5]
                || ((DIF_Header*)data)->exp[i].package[6]
                || ((DIF_Header*)data)->exp[i].package[7]))
            strlcpy(filename, rmc, 65536);
        else
            strlcpy(filename, pack, 65536);
        strlcat(filename, (char*)(data +
                bswap32(((DIF_Header*)data)->exp[i].name_offset)), 65536);
        
        FILE* expfile = fopen(filename, "rb");
        if (!expfile)
            return i + 1;
        
        fseek(expfile, 0, SEEK_END);
        expfilesize[i] = ftell(expfile);
        expdata[i] = (u8*)malloc(expfilesize[i]);
        assert(expdata[i]);
        fseek(expfile, 0, SEEK_SET);
        fread(expdata[i + 1], 1, expfilesize[i], expfile);
        fclose(expfile);
        
        printf("  finished reading expanded file %X (%s)!\n", ++i, filename);
    }
    
    return 0;
}

void ProcessDif(u8 *data, u32 size, FILE *file)
{
    u32 dataOffset = bswap32(((DIF_Header*)data)->header_size);
    u32 inputDataSize = bswap32(((DIF_Header*)data)->name_offset);

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
                return;
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
                --z;
                outdata.insert(outdata.end(),
                        expdata[z] + j, expdata[z] + j + y + 6);
                expcursors[z] = j + y + 6;
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
                --z;
                outdata.insert(outdata.end(),
                        expdata[z] + j, expdata[z] + j + y + 6);
                expcursors[z] = j + y + 6;
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
            y = (data[i] + 1) * 65536
                    + (data[i + 1] + 1) * 256 + data[i + 2];
            i += 3;
            j = expcursors[z];
            if (z)
            {
                --z;
                outdata.insert(outdata.end(),
                        expdata[z] + j, expdata[z] + j + y + 6);
                expcursors[z] = j + y + 6;
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
            y = (data[i] + 1) * 1677216 + (data[i + 1] + 1) * 65536
                    + (data[i + 2] + 1) * 256 + data[i + 3];
            i += 4;
            j = expcursors[z];
            if (z)
            {
                --z;
                outdata.insert(outdata.end(),
                        expdata[z] + j, expdata[z] + j + y + 6);
                expcursors[z] = j + y + 6;
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
            y = (data[i] + 1) * 65536
                    + (data[i + 1] + 1) * 256 + data[i + 2];
            i += 3;
            expcursors[z] -= (y + 1);
            break;
        }
        case 12: {
            z = b >> 4;
            y = (data[i] + 1) * 16777216 + (data[i + 1] + 1) * 65536
                    + (data[i + 2] + 1) * 256 + data[i + 3];
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
            expcursors[z] =
                    data[i] * 65536 + data[i + 1] * 256 + data[i + 2];
            i += 3;
            break;
        }
        default: {
            printf("  unknown command 2 at 0x%X!\n", i - 1);
            return;
        }
        }

        fwrite(outdata.data() + oldlen, outdata.size() - oldlen, 1, file);
        fflush(file);
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
