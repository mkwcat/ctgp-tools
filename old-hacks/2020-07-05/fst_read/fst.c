#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef unsigned int u32;

typedef struct
{
    u32 v1;
    u32 v2;
    u32 v3;
} fst_entry;

inline unsigned int tw_endian_swap_word(unsigned int v)
{
    return (( v & 0xFF ) << 24 )
    | (( v & (0xFF<<8) ) << 8 )
    | (( v & (0xFF<<16) ) >> 8 )
    | (( v & (0xFF<<24) ) >> 24 );
}


int main()
{
    FILE *fstf = fopen("fst.bin", "rb");
    fseek(fstf, 0, SEEK_END);
    int size = ftell(fstf);
    fseek(fstf, 0, SEEK_SET);
    char *fst = (char*)malloc(size);
    fread(fst, 1, size, fstf);
    fclose(fstf);

    u32 entry_count = tw_endian_swap_word(*(u32 *)(fst + 0x8));

    printf("FST entry count: %d\n\n", entry_count);

    fst_entry *entry = (fst_entry *) fst + 0xC;

    char *strtab = fst + (entry_count * 0xC);
    

    int i = 0;
    for(i = 0; i < 2204; i++)
    {
        u32 v1 = tw_endian_swap_word(entry->v1);
        u32 v2 = tw_endian_swap_word(entry->v2);
        u32 v3 = tw_endian_swap_word(entry->v3);

        if(v1>>24 == 0x01)
        {
            v1 &= 0xFFFFFF;
            char *str = strtab + v1;
            printf(" dir %d: %s\n", i, str);
        }
        else
        {
            char *str = strtab + v1;
            printf(" %d: %s -> %s\n", i, str, fst+v2);
        }
        

        

        
        //printf("%08x\n", str-(int)fst);
        

        entry++;
    }
}