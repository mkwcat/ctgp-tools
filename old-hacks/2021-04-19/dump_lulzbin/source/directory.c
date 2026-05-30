#include <gccore.h>
#include <wiiuse/wpad.h>

#include <fat.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <ogc/machine/processor.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

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

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------

	// Initialise the video system
	VIDEO_Init();

	// This function initialises the attached controllers
	WPAD_Init();

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

	if (!fatInitDefault()) {
		printf("fatInitDefault failure: terminating\n");
		goto error;
	}


	write16(0x0d8b420a, 2);
	saoPatchISFSAccess();

	ISFS_Initialize();

	s32 fd = ISFS_Open("/title/lulz.bin", 1);
	if (fd < 0) {
		printf("Could not open lulz!\n");
		goto error;
	}

	s32 ret = ISFS_Seek(fd, 0, SEEK_END);
	printf("size = %d\n", ret);
	ISFS_Seek(fd, 0, SEEK_SET);

	ISFS_Read(fd, (void*) 0x91000000, ret);

	FILE* f = fopen("/lulz.bin", "wb");
	fwrite((void*) 0x91000000, ret, 1, f);
	fclose(f);

	printf("done!\n");
	ISFS_Delete("/title/lulz.bin");
	printf("done!\n");

error:
	while(1) {

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

	return 0;
}
