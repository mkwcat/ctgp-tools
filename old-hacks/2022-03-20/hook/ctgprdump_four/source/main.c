#include <gccore.h>
#include <gctypes.h>
#include <wiiuse/wpad.h>

#include <network.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ogc/cache.h>
#include <ogc/machine/processor.h>
#include <ogc/es.h>
#include <ogc/isfs.h>

#include "miniz.h"

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

extern u8 boot1_dol[];
extern u32 boot1_dol_size;
extern u8 dump_dol[];
extern u32 dump_dol_size;

extern u8 apploader_bin[];
extern u32 apploader_bin_size;

u8 ios_dumper[] = {
	0xE3, 0xA0, 0x35, 0x36, 0xE5, 0x9F, 0x00, 0xAC, 0xE5, 0x83, 0x00, 0x24, 0xE2, 0x8F, 0x00, 0x54, 0xE3, 0xA0, 0x10, 0x03, 0xE6, 0x00, 0x03, 0x90, 0xE1, 0xA0, 0x40, 0x00, 0xE3, 0xA0, 0x15, 0x02, 0xE3, 0xA0, 0x25, 0x02, 0xE6, 0x00, 0x03, 0xF0, 0xE3, 0xA0, 0x35, 0x36, 0xE5, 0x9F, 0x00, 0x88, 0xE5, 0x83, 0x00, 0x24, 0xE1, 0xA0, 0x00, 0x04, 0xE3, 0xA0, 0x14, 0x11, 0xE3, 0xA0, 0x25, 0x02, 0xE6, 0x00, 0x03, 0xF0, 0xE3, 0xA0, 0x35, 0x36, 0xE5, 0x9F, 0x00, 0x70, 0xE5, 0x83, 0x00, 0x24, 0xE1, 0xA0, 0x00, 0x04, 0xE6, 0x00, 0x03, 0xB0, 0xE3, 0xA0, 0x35, 0x36, 0xE5, 0x9F, 0x00, 0x54, 0xE5, 0x83, 0x00, 0x24, 0xEA, 0xFF, 0xFF, 0xFE, 0x2F, 0x74, 0x69, 0x74, 0x6C, 0x65, 0x2F, 0x30, 0x30, 0x30, 0x31, 0x30, 0x30, 0x30, 0x31, 0x2F, 0x35, 0x33, 0x34, 0x31, 0x35, 0x30, 0x33, 0x30, 0x2F, 0x64, 0x61, 0x74, 0x61, 0x2F, 0x6D, 0x65, 0x6D, 0x31, 0x2E, 0x62, 0x69, 0x6E, 0x00, 0x2F, 0x74, 0x69, 0x74, 0x6C, 0x65, 0x2F, 0x30, 0x30, 0x30, 0x31, 0x30, 0x30, 0x30, 0x31, 0x2F, 0x35, 0x33, 0x34, 0x31, 0x35, 0x30, 0x33, 0x30, 0x2F, 0x64, 0x61, 0x74, 0x61, 0x2F, 0x6D, 0x65, 0x6D, 0x32, 0x2E, 0x62, 0x69, 0x6E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0xAA, 0xB5, 0x34, 0x01, 0x00, 0x94, 0xE1, 0x01, };

u8 replace_syscall[] = {
	0x48, 0x01, 0x49, 0x02, 0x60, 0x08, 0x47, 0x70, 0x12, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x94, 0xD8
};

inline uint32_t align_forward(uint32_t value, uint32_t align)
{
    return (value + align - 1) & (-1 ^ (align - 1));
}

inline uint32_t align_backward(uint32_t value, uint32_t align)
{
    return value & (-1 ^ (align - 1));
}

/* from rmc-local-net */
#define POKE_B(addr, dest)                                                     \
    do {                                                                       \
        *(uint32_t*) (addr)                                                    \
            = 0x48000000                                                       \
              + (((uint32_t)(dest) - (uint32_t)(addr)) & 0x3ffffff);           \
        DCFlushRange((void*) align_backward(addr, 32), 32);                    \
        ICInvalidateRange((void*) align_backward(addr, 32), 32);               \
    } while (0)
#define POKE_BL(addr, dest)                                                    \
    do {                                                                       \
        *(u32*) (addr)                                                    \
            = 0x48000001                                                       \
              + (((u32)(dest) - (u32)(addr)) & 0x3ffffff);           \
        DCFlushRange((void*) align_backward(addr, 32), 32);                    \
        ICInvalidateRange((void*) align_backward(addr, 32), 32);               \
    } while (0)

u32 main_start_sym = 0;

void boot1_main();
void chan_main();
void chan_start_thunk();
void patch_channel();


#define FS_MODULE_START 0x93A10000

static s32 saoPatchISFSAccess(void)
{
    static const u8 accessCheckBytes[] = 
    {
        0xD0, 0x01, 0x25, 0x66, 0x42, 0x6D, 0x1C, 0x28
    };
    u16* fsPtr;

    for (s32 i = 0;
         i < 0x2000;
         i += 2)
    {
        if (!memcmp((void*) FS_MODULE_START + i,
            accessCheckBytes, sizeof(accessCheckBytes)))
        {
            fsPtr = (u16*) (FS_MODULE_START + i);
            printf("Access patch: %08X\n", (u32) &fsPtr[3]);

            fsPtr[3] = 0x2000;  /* mov r0, #0 */
            DCFlushRange((void*) fsPtr, 2);

            return 0;
        }
    }
    return -1;
}


void chan_start()
{
	memcpy((void*) 0x81300000, (void*) apploader_bin, apploader_bin_size);
    DCFlushRange((void*) 0x81300000, apploader_bin_size);
    ((void (*)(u8*)) 0x81300000) (dump_dol);
#if 0
	patch_channel();
	((void (*)()) main_start_sym)();
#endif
}

u32 stub_start_sym = 0;

void chan_stub_start()
{
	POKE_B(0x80007cb0, chan_start_thunk);
	((void (*)()) stub_start_sym) ();
}

void chan_stub_patch(u8* data)
{
	stub_start_sym = *(u32*) (data + 0xE0);
	*(u32*) (data + 0xE0) = (u32) &chan_stub_start;
	((void (*)(u8*)) 0x81300000) (data);
}

void boot_start_hook()
{
	POKE_BL(0x80783de8, chan_stub_patch);

	boot1_main();
}


u8 alloc_data_d[0xC000];
u8* alloc_data;
u32 alloc_position = 0;

void* zmalloc(void *opaque, size_t items, size_t size)
{
	size *= items;
	alloc_position += size;
	if (size >= 0xC000)
		while (1) { }
	alloc_data += alloc_position;
	return alloc_data - size;
}

void zfree(void* a, void* b)
{
	return;
}


int main(int argc, char **argv)
{

	// Initialise the video system
	VIDEO_Init();

	// This function initialises the attached controllers
	//WPAD_Init();

	// Obtain the preferred video mode from the system
	// This will correspond to the settings in the Wii menu
	rmode = VIDEO_GetPreferredMode(NULL);

	// Allocate memory for the display in the uncached region
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	// Initialise the console, required for printf
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);

	// Set up the video registers with the chosen mode
	VIDEO_Configure(rmode);

	// Tell the video hardware where our display memory is
	VIDEO_SetNextFramebuffer(xfb);

	// Make the display visible
	VIDEO_SetBlack(FALSE);

	// Flush the video register changes to the hardware
	VIDEO_Flush();

	// Wait for Video setup to complete
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();


	// The console understands VT terminal escape codes
	// This positions the cursor on row 2, column 0
	// we can use variables for this with format codes too
	// e.g. printf ("\x1b[%d;%dH", row, column );
	printf("\x1b[2;0H");

#if 0
	DCInvalidateRange((void*) 0x90100000, 32);
    write32(0x00100000, 0);
    printf("val: %08X\n", read32(0x00100000));
	sleep(1);
    write8(0x00100001, 0x99);
	printf("sleep\n");
	sleep(1);
    printf("val: %08X\n", read32(0x00100000));
    sleep(1);
#endif

  s32 do_patch = 1;
#if 0
    s32 fd = IOS_Open("/ctgpr.dol", ISFS_OPEN_READ);
	if (fd >= 0) {
		s32 dsize = IOS_Seek(fd, 0, SEEK_END);
		IOS_Seek(fd, 0, SEEK_SET);
		IOS_Read(fd, (void*) 0x91000000, dsize);
		IOS_Close(fd);
	}
#endif

	printf("setup ios dumper\n");
	// replace boot_new_ios_kernel
	memcpy((void*) 0x92000000, ios_dumper, sizeof(ios_dumper));
	DCFlushRange((void*) 0x92000000, 0x400);
	// replace boot_new_ios_kernel
	*(unsigned int*) 0xCD4F9358 = 0x12000000;

	printf("done, not make file...\n");

	printf("delete and remake file\n");

	saoPatchISFSAccess();
	ISFS_Initialize();

	ISFS_Delete("/title/00010001/53415030/data/mem1.bin");
	ISFS_CreateFile("/title/00010001/53415030/data/mem1.bin", 0, 3, 3, 3);

#if 0
	__IOS_LaunchNewIOS(58);
	while (1) { }
#endif

    z_stream stream;
    // Init the z_stream
  	memset(&stream, 0, sizeof(stream));
  	stream.next_in = boot1_dol;
  	stream.avail_in = boot1_dol_size;
  	stream.next_out = (u8*) 0x80004000;
  	stream.avail_out = 0x100000;
  stream.zalloc = &zmalloc;
  stream.zfree = &zfree;
  alloc_data = alloc_data_d;

  if (inflateInit(&stream))
    {
		while (1) { }
    }
	inflate(&stream, Z_SYNC_FLUSH);

    memcpy((void*) 0x81300000, (void*) apploader_bin, apploader_bin_size);
    DCFlushRange((void*) 0x81300000, apploader_bin_size);

	SYS_ResetSystem(SYS_SHUTDOWN, 0, 0);
    ((void (*)(u32)) 0x81300000) (0x80004000);
#if 0
error:
	while(1)
    {
		// Call WPAD_ScanPads each loop, this reads the latest controller states
		WPAD_ScanPads();

		// WPAD_ButtonsDown tells us which buttons were pressed in this loop
		// this is a "one shot" state which will not fire again until the button has been released
		u32 pressed = WPAD_ButtonsDown(0);

		// We return to the launcher application via exit
		if ( pressed & WPAD_BUTTON_HOME ) exit(0);

		// Wait for the next frame
		VIDEO_WaitVSync();
	}
#endif
	return 0;
}
