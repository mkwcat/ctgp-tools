#include <cassert>
#include <cstdio>
#include <cstring>

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

#define BACK_BUF_SIZE 0x1000
#define HASH_MAP_SIZE 0x4000

struct Yaz_file_struct
{
    FILE* file;
    u32 _0x4;
    int _0x8;
    u32 backBufferLocation; // C
    u8 backBuffer[BACK_BUF_SIZE];
    u32 _0x1010;
    u32 byteShifterRemaining;
    u32 _0x1018;
    u16 hashMap[HASH_MAP_SIZE]; // 101c
    int copyLocation; // 901c
    u32 copyDistance; // 9020
    u32 copyLength;
    u32 _0x9028;
    u8 _0x902C[0x922C - 0x902C];
    u8 codeBufferLocation; // 922c
    u8 codeBufferIndex; // 922d
    u8 codeBuffer[0x9248 - 0x922E]; // not actual size probably
};

u16 DAT_80ad1160[HASH_MAP_SIZE];

u32 hash1(u32 value)
{
    assert(!(value & 0xff000000));

    // This is... some kind of hash I guess
    // return ((value * value * 0xEF34 + value + 0xB205) >> 10) &
    //       (HASH_MAP_SIZE - 1);
    return value + 0xb205 + value * value * 0xef34 >> 0xa & 0x3fff;
}

static inline u32 readFromIndex(Yaz_file_struct* file, u32 index)
{
    return file->backBuffer[index & (BACK_BUF_SIZE - 1)] |
           (u32(file->backBuffer[(index + 1) & (BACK_BUF_SIZE - 1)]) << 8) |
           (u32(file->backBuffer[(index + 2) & (BACK_BUF_SIZE - 1)]) << 16);
}

int hashLookup(Yaz_file_struct* file, u32 data)
{
    u32 h1 = hash1(data);
    assert(h1 < HASH_MAP_SIZE);

    u32 count = 0;
    for (u32 res = 0; (res = file->hashMap[h1]) & 0x8000;
         h1 = (h1 + 1) & (HASH_MAP_SIZE - 1))
    {
        if (res & 0x4000)
        {
            assert(res & 0x8000);
            if (readFromIndex(file, res & 0xFFF) == data)
                return res & 0xFFF;
        }

        assert(++count <= HASH_MAP_SIZE);
    }
    return -1;
}

void hashCleanup(Yaz_file_struct* file)
{
    // Really have no clue what's going on in here

    memset(DAT_80ad1160, 0xFF, sizeof(DAT_80ad1160));

    u16* hashMap = file->hashMap;

    for (u32 hi = 0; hi < HASH_MAP_SIZE; hi++)
    {
        if ((!(hashMap[hi] & 0x8000)) || (hashMap[hi] & 0x4000))
            continue;

        u32 hg = hi;
        u32 r18 = hg;
        do
        {
            r18 = (r18 + 1) & (HASH_MAP_SIZE - 1);
        } while (hashMap[r18] & 0x8000);

        do
        {
            assert(!(hashMap[hg] & 0x4000));
            hashMap[hg] = 0x8000;
            DAT_80ad1160[hg] = 0xFFFF;

            u32 uVar11 = r18;
            u16 res = 0;
            u16 hh = 0;
            while (true)
            {
                uVar11 = (uVar11 - 1) & (HASH_MAP_SIZE - 1);
                if (uVar11 == hg)
                    break;

                res = hashMap[uVar11];
                assert(res & 0x8000);

                if ((res & 0x4000) == 0)
                {
                    // hh = DAT_80ad1160[uVar11];
                    // if (hh == 0xFFFF)
                    // {
                    hh = hash1(readFromIndex(file, res & 0xFFF));
                    // DAT_80ad1160[uVar11] = hh;
                    //}
                    assert(hh < HASH_MAP_SIZE);

                    if (((uVar11 - hh) & (HASH_MAP_SIZE - 1)) >=
                        ((hg - hh) & (HASH_MAP_SIZE - 1)))
                        break;
                }
            }

            if (uVar11 == hg)
                break;

            hashMap[hg] = hashMap[uVar11];
            // DAT_80ad1160[hg] = DAT_80ad1160[uVar11];
            hashMap[uVar11] = 0x8000;
            // DAT_80ad1160[uVar11] = 0xFFFF;
            hg = uVar11;
        } while (true);
    }

    // printf("retn\n");
}

bool hashInsert(Yaz_file_struct* file, u32 index)
{
    u32 value = readFromIndex(file, index);

    for (u32 i = 0; i < 4; i++)
    {
        if (readFromIndex(file, index - 4 + i) == value)
            return false;
    }

    u32 h1 = hash1(value);
    assert(h1 < HASH_MAP_SIZE);

    u32 count = 0;
    u16 res = 0;
    while (true)
    {
        u16 res = file->hashMap[h1];

        if (!(res & 0x4000))
            break;

        assert(++count <= HASH_MAP_SIZE);

        h1 = (h1 + 1) & (HASH_MAP_SIZE - 1);
    }

    file->hashMap[h1] = index | 0xC000;
    return true;
}

// This is inlined so the function bounds and arguments are a guess
bool hashRemove(Yaz_file_struct* file, u32 index)
{
    u32 value = readFromIndex(file, index);

    u32 h1 = hash1(value);
    assert(h1 < HASH_MAP_SIZE);

    u32 count = 0;
    while (true)
    {
        u16 res = file->hashMap[h1];

        assert(res & 0x8000);

        if ((res & 0x4000) && ((res & 0xFFF) == index))
            break;

        assert(++count <= HASH_MAP_SIZE);

        h1 = (h1 + 1) & (HASH_MAP_SIZE - 1);
    }

    u32 i = 1;
    for (; i < 5; i++)
    {
        if (readFromIndex(file, index + i) == value)
        {
            file->hashMap[h1] = ((index + i) & (HASH_MAP_SIZE - 1)) | 0xC000;
            break;
        }
    }
    if (i >= 5)
    {
        file->hashMap[h1] = 0x8000;
        return true;
    }

    return false;
}

int Yaz_buffer_putcode_r(int* reent, Yaz_file_struct* file)
{
#if 0
    assert(file->codeBufferLocation);

    if (file->codeBufferLocation + file->_0x9028 > 0x200)
    {
        // The order of code here is a bit strange, this way matches it the most
        u32 wrote = 0;
        u32 toWrite = file->_0x9028;
        for (int ret = 0; ret >= 0;)
        {

            ret = fwrite(&file->_0x902C[wrote], 1, toWrite - wrote, file->file);
            wrote += ret;
            if (wrote >= toWrite)
                break;
        }

        if (wrote != toWrite)
        {
            // ignoring errno
            *reent = -1;
            return -1;
        }
    }

    memcpy(file->_0x902C + file->_0x9028, file->codeBuffer,
           file->codeBufferLocation);
    file->_0x9028 += file->codeBufferLocation;
#endif
    fwrite(file->codeBuffer, file->codeBufferLocation, 1, file->file);
    fflush(file->file);

    return 0;
}

static void resetCodeBuffer(Yaz_file_struct* file)
{
    file->codeBufferIndex = 8;
    file->codeBuffer[0] = 0;
    file->codeBuffer[1] = 1;
    file->codeBufferLocation = 1;
}

bool Yaz_open(Yaz_file_struct* file, const char* path)
{
    memset(file, 0, sizeof(Yaz_file_struct));

    file->file = fopen(path, "wb");
    if (!file->file)
        return false;

    const u8 szsHeader[] = {
        0x59, 0x61, 0x7A, 0x30, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };

    int ret = fwrite(szsHeader, sizeof(szsHeader), 1, file->file);
    assert(ret == 1);

    file->copyLocation = -1;
    return true;
}

int Yaz_fputc_r(int* reent, int value, Yaz_file_struct* file)
{
    // printf("%08X\n", file->backBufferLocation);

    if (value != -1)
    {
        file->byteShifterRemaining++;
        file->_0x8++;
        file->_0x1010 = value << 24 | file->_0x1010 >> 8;

        if (file->byteShifterRemaining < 3)
            return value;

        if ((file->_0x8 - 0xFFF) > 6)
        {
            file->_0x1018 += hashRemove(file, (file->backBufferLocation + 1) &
                                                  (BACK_BUF_SIZE - 1))
                                 ? 1
                                 : 0;
        }

        if (file->_0x1018 > 1365)
        {
            hashCleanup(file);
            file->_0x1018 = 0;
        }

        if (file->_0x8 > 9)
        {
            file->_0x1018 -= hashInsert(file, (file->backBufferLocation - 3) &
                                                  (BACK_BUF_SIZE - 1))
                                 ? 1
                                 : 0;
        }

        // 807eaf4c
        if (file->copyLocation != -1)
        {
            if (((file->_0x1010 >> 8) & 0xFF) ==
                file->backBuffer[file->copyLocation])
            {
                file->backBuffer[file->backBufferLocation] = file->_0x1010 >> 8;
                file->backBufferLocation =
                    (file->backBufferLocation + 1) & (BACK_BUF_SIZE - 1);
                file->byteShifterRemaining -= 1;
                file->copyLocation =
                    (file->copyLocation + 1) & (BACK_BUF_SIZE - 1);
                file->copyLength++;
                assert(file->copyDistance ==
                       ((file->backBufferLocation - file->copyLocation - 1) &
                        (BACK_BUF_SIZE - 1)));
                return value;
            }

            // 807eaf78 - 807eb1a8
            if (file->copyLength > 3)
            {
                do
                {
                    // 807eaf84
                    if (file->codeBufferIndex == 0)
                        resetCodeBuffer(file);

                    assert((file->copyDistance & ~0xFFF) == 0);

                    u32 copyLen = file->copyLength;

                    if (copyLen > 0x11)
                    {
                        // Long copy
                        if (copyLen > 0x111)
                            copyLen = 0x111;

                        file->codeBuffer[file->codeBufferLocation] =
                            file->copyDistance >> 8;
                        file->codeBuffer[file->codeBufferLocation + 1] =
                            file->copyDistance & 0xFF;
                        file->codeBuffer[file->codeBufferLocation + 2] =
                            copyLen - 0x12;
                        file->codeBufferLocation += 3;
                        file->codeBufferIndex--;
                    }
                    else
                    {
                        file->codeBuffer[file->codeBufferLocation] =
                            ((copyLen - 2) << 4) | (file->copyDistance >> 8);
                        file->codeBuffer[file->codeBufferLocation + 1] =
                            file->copyDistance & 0xFF;
                        file->codeBufferLocation += 2;
                        file->codeBufferIndex--;
                    }

                    if (file->codeBufferIndex == 0 &&
                        Yaz_buffer_putcode_r(reent, file) != 0)
                    {
                        assert(!"Yaz_buffer_putcode_r error!");
                        return -1;
                    }

                    file->copyLength -= copyLen;
                } while (file->copyLength > 2);
            }
        }
    }

    // 807eb1a8
    while (file->copyLength > 0)
    {
        assert(file->copyLocation != -1);

        if (file->codeBufferIndex == 0)
            resetCodeBuffer(file);
        file->codeBuffer[file->codeBufferLocation] =
            file->backBuffer[(file->backBufferLocation - file->copyLength) &
                             (BACK_BUF_SIZE - 1)];
        file->codeBufferIndex--;
        file->codeBuffer[0] |= 1 << file->codeBufferIndex;
        file->codeBufferLocation++;

        file->copyLength--;

        // printf("coplen %d\n", file->copyLength);
        if (file->codeBufferIndex == 0 &&
            Yaz_buffer_putcode_r(reent, file) != 0)
        {
            assert(!"Yaz_buffer_putcode_r error!");
            return -1;
        }
        // printf("done write\n");
    }

    // printf("loop break!\n");

    if (value == -1)
    {
        for (; file->byteShifterRemaining > 0; file->byteShifterRemaining--)
        {
            file->backBuffer[file->backBufferLocation] =
                file->_0x1010 >> (file->byteShifterRemaining * -8 + 32);

            file->backBufferLocation =
                (file->backBufferLocation + 1) & (BACK_BUF_SIZE - 1);

            if (file->codeBufferIndex == 0)
                resetCodeBuffer(file);

            file->codeBuffer[file->codeBufferLocation] =
                file->backBuffer[file->_0x1010 >>
                                 (file->byteShifterRemaining * -8 + 32)];
            file->codeBufferIndex--;
            file->codeBuffer[0] |= 1 << file->codeBufferIndex;
            file->codeBufferLocation++;

            if (file->codeBufferIndex == 0 &&
                Yaz_buffer_putcode_r(reent, file) != 0)
            {
                assert(!"Yaz_buffer_putcode_r error!");
                return -1;
            }
        }

        if (file->codeBufferIndex == 0 &&
            Yaz_buffer_putcode_r(reent, file) != 0)
        {
            assert(!"Yaz_buffer_putcode_r error!");
            return -1;
        }

        if (file->_0x9028)
        {
            printf("returning 0!\n");
            return 0;
        }

        int ret = fwrite(file->_0x902C, 1, file->_0x9028, file->file);
        if (ret == file->_0x9028)
        {
            printf("returning 0!\n");
            return 0;
        }

        assert(!"fwrite error!");
        return -1;
    }

    assert(file->byteShifterRemaining == 3);
    file->backBuffer[file->backBufferLocation++] = file->_0x1010 >> 8;
    file->backBufferLocation &= (BACK_BUF_SIZE - 1);

    file->byteShifterRemaining = 2;

    if (file->_0x8 >= 3)
    {
        int index = hashLookup(file, file->_0x1010 >> 8);

        if (index >= 0)
        {
            assert(file->backBuffer[(file->backBufferLocation - 1) &
                                    (BACK_BUF_SIZE - 1)] ==
                   file->backBuffer[index & (BACK_BUF_SIZE - 1)]);
            assert(index !=
                   ((file->backBufferLocation - 1) & (BACK_BUF_SIZE - 1)));

            file->copyLength = 1;
            file->copyLocation = (index + 1) & (BACK_BUF_SIZE - 1);
            file->copyDistance =
                (file->backBufferLocation - 1 - file->copyLocation) &
                (BACK_BUF_SIZE - 1);
            // printf("%d  copyLength %d, copyLocation %d, copyDistance %d\n",
            //        file->_0x8, file->copyLength, file->copyLocation,
            //        file->copyDistance);
            return value;
        }
    }

    if (file->codeBufferIndex == 0)
        resetCodeBuffer(file);

    file->codeBufferIndex--;
    file->codeBuffer[0] |= 1 << file->codeBufferIndex;
    file->codeBuffer[file->codeBufferLocation++] = file->_0x1010 >> 8;

    if (file->codeBufferIndex == 0 && Yaz_buffer_putcode_r(reent, file) != 0)
    {
        assert(!"Yaz_buffer_putcode_r error!");
        return -1;
    }

    file->copyDistance = 0;
    file->copyLength = 0;
    file->copyLocation = -1;

    u32 pos = (file->backBufferLocation - 1) & (BACK_BUF_SIZE - 1);
    if (file->backBuffer[pos] == ((file->_0x1010 >> 16) & 0xFF))
    {
        file->copyLocation = pos;
    }

    pos = (file->backBufferLocation - 2) & (BACK_BUF_SIZE - 1);
    if (file->backBuffer[pos] == ((file->_0x1010 >> 16) & 0xFF))
    {
        file->copyLocation = pos;
        file->copyDistance = 1;
    }

    return value;
}

int main()
{
    Yaz_file_struct file;
    bool ret = Yaz_open(&file, "./Globe.u8.reyaz");
    assert(ret);
    printf("opened write!\n");

    FILE* u8_file = fopen("./Globe.u8", "rb");
    if (!u8_file)
    {
        printf("failed to open u8 file!!\n");
        return 1;
    }

    fseek(u8_file, 0, SEEK_END);
    u32 u8_size = ftell(u8_file);
    u8* u8_data = (u8*)malloc(u8_size);
    assert(u8_data);
    fseek(u8_file, 0, SEEK_SET);
    fread(u8_data, 1, u8_size, u8_file);
    fclose(u8_file);

    FILE* fo = fopen("debug.bin", "wb");

    printf("calling!\n");
    int reent;
    for (u32 i = 0; i < u8_size; i++)
    {
        fwrite(&u8_data[i], 1, 1, fo);
        fflush(fo);
        Yaz_fputc_r(&reent, u8_data[i], &file);
    }
    printf("done!\n");

    Yaz_fputc_r(&reent, -1, &file);
    printf("final done!\n");

    fclose(file.file);
}
