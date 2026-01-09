

typedef unsigned int u32;
typedef unsigned short u16;


#define SEARCH_LENGTH 0x300000

u32 *search_fopen(u32 start)
{
    int i = SEARCH_LENGTH;
    u32 *cur = (u32 *) start;

    do
    {
        cur++;

        if(cur[0] == 0x7c601b78)
        {
            if(cur[2] == 0x7c852378)
            {
                if(cur[3] == 0x7c040378)
                {
                    if((*(char *)&cur[4] == 0x4b) || (*(char *)&cur[4] == 0x48))
                    {
                        return cur - 1;
                    }
                }
            }
        }
    } while (--i);
    
    return 0;
}

u32 *search_fwrite(u32 start)
{
    int i = SEARCH_LENGTH;
    u32 *cur = (u32 *) start;

    do
    {
        cur++;

        if(cur[0] == 0x7C681B78)
        {
            if(cur[1] == 0x7C8A2378)
            {
                if(cur[2] == 0x7CA92B78)
                {
                    if(cur[4] == 0x7CC73378)
                    {
                        if(cur[5] == 0x7D044378)
                        {
                            return cur - 1;
                        }
                    }
                }
            }
        }
    } while (--i);
    
    return 0;
}

u32 *fopen = 1;
u32 *fwrite = 1;
u16 fopen_thing_ha = 1;

int fopen_hook(char *name, char *access);
void setup()
{
    #define B_GEN(SRC, DST) ((u32)SRC < (u32)DST ? (((u32)DST-(u32)SRC) | 0x48000000) : (((u32)DST-(u32)SRC) ^ 0xB4000000))

    register u32 ctr;
    asm volatile(
        "mfctr %0\n" : "=r" (ctr)
    );

    

    fopen = search_fopen(ctr);
    fwrite = search_fwrite(ctr);

    fopen_thing_ha = (u16 *)fopen[1];




    *fopen = B_GEN(fopen, &fopen_hook);

    asm volatile(
        "mtctr %0\n bctrl\n" : "=r" (ctr)
    );
}

int call_original_fopen(char *name, char *access);
int call_fwrite(int file, u32 *data, u32 size);

int called = 0;

int fopen_hook(char *name, char *access)
{
    if(called)
    {
        return call_original_fopen(name, access);
    }

    called = 1;

    u32 *dol = 0x91000000;

    int file = call_original_fopen("sda:/ctgpdump.bin", "w");
    call_fwrite(file, dol, 0x0010cc0e);


    return call_original_fopen(name, access);
}