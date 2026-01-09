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
	0xE5, 0x9F, 0x40, 0x50, 0xE5, 0x9F, 0x00, 0x50, 0xE5, 0x84, 0x00, 0x00, 0xE3, 0xA0, 0x04, 0x12, 0xE5, 0x9F, 0x10, 0x48, 0xE6, 0x00, 0x03, 0x90, 0xE3, 0x50, 0x00, 0x00, 0xB5, 0x9F, 0x10, 0x40, 0xA5, 0x9F, 0x10, 0x40, 0xE5, 0x84, 0x10, 0x00, 0xBA, 0xFF, 0xFF, 0xFE, 0xE1, 0xA0, 0x50, 0x00, 0xE3, 0xA0, 0x10, 0x00, 0xE3, 0xA0, 0x25, 0x06, 0xE6, 0x00, 0x03, 0xF0, 0xE5, 0x9F, 0x10, 0x28, 0xE5, 0x84, 0x10, 0x00, 0xE1, 0xA0, 0x00, 0x05, 0xE6, 0x00, 0x03, 0xB0, 0xE5, 0x9F, 0x10, 0x1C, 0xE5, 0x84, 0x10, 0x00, 0xEA, 0xFF, 0xFF, 0xFE, 0x0d, 0x80, 0x00, 0x24, 0x54, 0xFF, 0x4C, 0x01, 0x00, 0x00, 0x06, 0x02, 0x2B, 0x15, 0x95, 0x01, 0xFF, 0x6B, 0x1D, 0x01, 0x00, 0x94, 0xE1, 0x01, 0x80, 0x80, 0xFF, 0x01, 
	};

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

#define MEM2_PROT          0x0D8B420A
#define ES_MODULE_START (u16*)0x939F0000

static const u16 ticket_check[] = {
    0x685B,               // ldr r3,[r3,#4] ; get TMD pointer
    0x22EC, 0x0052,       // movls r2, 0x1D8
    0x189B,               // adds r3, r3, r2; add offset of access rights field in TMD
    0x681B,               // ldr r3, [r3]   ; load access rights (haxxme!)
    0x4698,               // mov r8, r3  ; store it for the DVD video bitcheck later
    0x07DB                // lsls r3, r3, #31; check AHBPROT bit
};

static int patch_ahbprot_reset(void)
{
	u16 *patchme;

	//if ((read32(0x0D800064) == 0xFFFFFFFF) ? 1 : 0) {
		write16(MEM2_PROT, 2);
		for (patchme=ES_MODULE_START; patchme < ES_MODULE_START+0x4000; ++patchme) {
			if (!memcmp(patchme, ticket_check, sizeof(ticket_check)))
			{
				// write16/uncached poke doesn't work for MEM2
				patchme[4] = 0x23FF; // li r3, 0xFF
				DCFlushRange(patchme+4, 2);
				return 0;
			}
		}
		return -1;
	//} else {
	//	return -2;
	//}
}

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

extern u8 filemodule_elf[];
extern u32 filemodule_elf_size;

extern u32 es_bin[];
extern u32 es_bin_size;

#define FILE_MODULE_NAME "file"
#define FILE_MODULE_NAME_LENGTH 4

typedef struct _stats
{
	u64 Identifier; // st_ino; on fat, cluster number.
	u64 Size;
	s32 Device;
	s32 Mode;
} Stats;

typedef enum {
	SD_DISK,
	USB_DISK,
	USB2_DISK,
	DISK_NONE
} disk_phys;

typedef enum {
	FS_FAT,
	FS_ELM,
	FS_NTFS,
	FS_EXT2,
	FS_SMB,
	FS_ISFS,
	FS_RIIFS
} disk_fs;

typedef enum {
	IOCTL_InitDisc =	0x30,
	IOCTL_Mount,
	IOCTL_Unmount,
	IOCTL_MountPoint,
	IOCTL_SetDefault,
	IOCTL_SetLogFS,
	IOCTL_GetLogFS,
	IOCTL_Epoch,
	IOCTL_Stat =		0x40,
	IOCTL_CreateFile,
	IOCTL_Delete,
	IOCTL_Rename,
	SEEK_Tell,
	SEEK_Sync,
	IOCTL_CreateDir =	0x50,
	IOCTL_OpenDir,
	IOCTL_NextDir,
	IOCTL_CloseDir,
	IOCTL_Shorten = 	0x60,
	IOCTL_Log =         0x61,
	IOCTL_Context =     0x62,
	IOCTL_CheckPhys,
	IOCTL_GetFreeSpace,
	IOCTL_SetSlotLED,
} file_ioctl;

typedef enum {
	ERROR_SUCCESS = 		0,
	ERROR_NOTOPENED = 		-0x40,
	ERROR_OUTOFMEMORY = 	-0x41,
	ERROR_UNRECOGNIZED =	-0x80,
	ERROR_NOTMOUNTED =		-0x81,
	ERROR_DISKNOTSTARTED =	-0x90,
	ERROR_DISKNOTINSERTED = -0x91,
	ERROR_DISKNOTMOUNTED =	-0x92
} file_error;

static u32 ioctlbuffer[0x20];
static int file_fd = -1;

#define os_open IOS_Open
#define os_close IOS_Close
#define os_read IOS_Read
#define os_write IOS_Write
#define os_ioctl IOS_Ioctl
#define os_ioctlv IOS_Ioctlv
#define os_seek IOS_Seek
// libogc will take care of syncing
#define os_sync_after_write(a,b)
#define os_sync_before_read(a,b)
#define Alloc malloc
#define Realloc(a, b, c) realloc(a, b)
#define Dealloc free

int File_Init()
{
	if (file_fd < 0) {
		file_fd = os_open(FILE_MODULE_NAME, 0);
		if (file_fd >= 0) {
			time_t epoch = time(NULL);
			memcpy(ioctlbuffer, &epoch, sizeof(time_t));
			os_ioctl(file_fd, IOCTL_Epoch, ioctlbuffer, sizeof(time_t), NULL, 0);
		}
	}

	return file_fd;
}

int File_Deinit()
{
	if (file_fd >= 0)
		os_close(file_fd);
	file_fd = -1;

	return 0;
}

static int FileMountDisk(disk_phys disk)
{
	ioctlbuffer[0] = disk;
	os_sync_after_write(ioctlbuffer, 0x04);
	return os_ioctl(file_fd, IOCTL_InitDisc, ioctlbuffer, sizeof(ioctlbuffer), NULL, 0);
}


int File_Mount(disk_fs filesystem, const void* options, int optionslen)
{
	if (file_fd < 0)
		return ERROR_NOTOPENED;

	ioctlbuffer[0] = filesystem;
	os_sync_after_write(ioctlbuffer, 0x04);
	os_sync_after_write((void*)options, optionslen);
	return os_ioctl(file_fd, IOCTL_Mount, ioctlbuffer, sizeof(ioctlbuffer), (void*)options, optionslen);
}

int File_SetDefault(int fs)
{
	ioctlbuffer[0] = fs;
	os_sync_after_write(ioctlbuffer, 0x04);
	return os_ioctl(file_fd, IOCTL_SetDefault, ioctlbuffer, sizeof(ioctlbuffer), NULL, 0);
}

int File_Fat_Mount(disk_phys disk, const char* name)
{
	int ret = FileMountDisk(disk);
	if (ret < 0)
		return ret;

	char options[0x40];
	memcpy(options, &ret, sizeof(u32));
	strncpy(options + 4, name, 0x40 - 4);
	return File_Mount(FS_FAT, options, 4 + strlen(name) + 1);
}

/* Debugging thread */
lwp_t ut_h = (lwp_t) NULL;

void* thread_proc(void* arg)
{
    printf("debug thread entry\n");

    sleep(4);
	printf("abc: %d\n", read32(0x34));
	while (1) { }
    return NULL;
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


#if 0
  u8 mac[6];
  net_init();
  if (net_get_mac_address(mac) < 0)
	do_patch = 0;
  else if (memcmp((void*) system_mac, (void*) mac, 6))
	do_patch = 0;
  net_deinit();
#endif

#if 0
    s32 fd = IOS_Open("/ctgpr.dol", ISFS_OPEN_READ);
	if (fd >= 0) {
		s32 dsize = IOS_Seek(fd, 0, SEEK_END);
		IOS_Seek(fd, 0, SEEK_SET);
		IOS_Read(fd, (void*) 0x91000000, dsize);
		IOS_Close(fd);
	}
#endif

	printf("a: %d\n", patch_ahbprot_reset());
	__IOS_LaunchNewIOS(37);
	//sleep(1);
	write16(0x0d8b420a, 2);
	__ES_Init();

#if 0
	printf("make thread\n");
    LWP_CreateThread(&ut_h, thread_proc, 0, 0, 256, 50);
    LWP_SetThreadPriority(LWP_GetSelf(), 50);
#endif

	printf("setup ios dumper\n");
	u32* file = (u32*) 0x91000000;
    memset((void*) file, 0, 32);

    file[0] = 0x46494C45;
    file[1] = filemodule_elf_size;
    memcpy((void*) 0x91000020, filemodule_elf, filemodule_elf_size);
    DCFlushRange((void*) 0x91000000, filemodule_elf_size + 32);

	/* I cba to explain what's going on here */
#if 0
    write32(0x939FB738, 0x49004708);
    write32(0x939FB73C, (u32)(&es_bin) - 0x80000000);
    write32(0x34, 0x12345678);
#else
	printf("%08X\n%08X\n", read32(0x139FA8E8), read32(0x139FA8EC));
	write32(0x139FA8E8, 0x49004708);
    write32(0x139FA8EC, (u32)(&es_bin) - 0x80000000);
    write32(0x34, 0x12345678);
#endif

	printf("do\n");
    u8 devicecert[512] ATTRIBUTE_ALIGN(32); // probably big enough
    s32 ret = ES_GetDeviceCert(devicecert);
    printf("ES_GetDeviceCert() = %d\n", ret);
	if (ret < 0) while (1) { }

	sleep(1);

	ret = File_Init();
	printf("File_Init() = %d\n", ret);
	if (ret < 0) while (1) { }

	ret = File_Fat_Mount(SD_DISK, "sd");
	printf("mount = %d\n", ret);
	if (ret < 0) while (1) { }
	ret = File_SetDefault(ret);
	printf("default = %d\n", ret);
	if (ret < 0) while (1) { }

	memcpy((void*) 0x92000000, "file/ctgpr_mem1_dump.raw", sizeof("file/ctgpr_mem1_dump.raw"));
	DCFlushRange((void*) 0x92000000, 32);


	//printf("open 1: %d\n", IOS_Open("file/aaaaaa.bin", 0x602));
	//printf("open 2: %d\n", IOS_Open("file/mnt/usb/bbbbbb.bin", 0x602));



	// replace boot_new_ios_kernel
	for (s32 i = 0; i < sizeof(ios_dumper); i += 4) {
		write32(0x0D4FF100 + i, *(u32*)(ios_dumper + i));
	}
	printf("copy code done\n");

	//write32(0x0D4F94D8, 0xFFFFF100);
	write32(0x0D4f92b8, 0xFFFFF100);
	sleep(1);
	printf("starting...\n");

#if 0
	__IOS_LaunchNewIOS(58);
	printf("?\n");
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
