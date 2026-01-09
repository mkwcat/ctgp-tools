
#define syscall .word 0xE6000010 | 0x20 *

#define IOS_CreateThread             0x00
#define IOS_GetThreadId              0x03
#define IOS_StartThread              0x05
#define IOS_SuspendThread            0x06
#define IOS_CreateMessageQueue       0x0A
#define IOS_DestroyMessageQueue      0x0B
#define IOS_SendMessage              0x0C
#define IOS_ReceiveMessage           0x0E
#define IOS_RegisterResourceManager  0x1B
#define IOS_ResourceReply            0x2A
#define IOS_InvalidateDCache         0x3F
#define IOS_FlushDCache              0x40
#define IOS_LaunchElf                0x41
#define IOS_LaunchRM                 0x5A

#define IOS_Open 0x1C
#define IOS_Close 0x1D
#define IOS_Read 0x1E
#define IOS_Write 0x1F

#define VFILE_BASE                   0x11000000
#define VFILE_MAGIC                  0x46494C45
#define VFILE_MAXSIZE                0x01000000
#define FILE_MAGIC                   0x00
#define FILE_SIZE                    0x04
#define FILE_OPEN                    0x08
#define FILE_POS                     0x0C
#define FILE_DATA                    0x20

#define MSG_COMMAND                  0x00
#define MSG_RESULT                   0x04
#define MSG_FD                       0x08
#define MSG_OPEN_PATH                0x0C
#define MSG_OPEN_MODE                0x10
#define MSG_OPEN_UID                 0x14
#define MSG_OPEN_GID                 0x18
#define MSG_RW_DATA                  0x0C
#define MSG_RW_LENGTH                0x10
#define MSG_SEEK_WHERE               0x0C
#define MSG_SEEK_WHENCE              0x10
#define MSG_IOCTL_COMMAND            0x0C
#define MSG_IOCTL_IN                 0x10
#define MSG_IOCTL_IN_LEN             0x14
#define MSG_IOCTL_OUT                0x18
#define MSG_IOCTL_OUT_LEN            0x1C

#define IOS_OPEN                     1
#define IOS_CLOSE                    2
#define IOS_READ                     3
#define IOS_WRITE                    4
#define IOS_SEEK                     5
#define IOS_IOCTL                    6
#define IOS_IOCTLV                   7

#define IOS_OPEN_NONE                0
#define IOS_OPEN_READ                1
#define IOS_OPEN_WRITE               2
#define IOS_OPEN_RW                  (IOS_OPEN_READ | IOS_OPEN_WRITE)

#define SEEK_SET                     0
#define SEEK_CUR                     1
#define SEEK_END                     2

#define ISFS_STATS_SIZE              0x00
#define ISFS_STATS_POS               0x04

#define IOS_SUCCESS                  0
#define IOS_ENOENT                   -6
#define ISFS_EINVAL                  -101
#define ISFS_EACCESS                 -102

.arm

// set YUV green color
mov r3, #0x0d800000
ldr r0, =0x101
str r0, [r3, #0x24]

adr r0, file_str
mov r1, #0x3
syscall IOS_Open
mov r4, r0

ldr r1, =0x00800000
ldr r2, =0x00800000
syscall IOS_Write

// set YUV purple
mov r3, #0x0d800000
ldr r0, =((170 << 24) | (181 << 16) | (52 << 8)) | 1
str r0, [r3, #0x24]

mov r0, r4
ldr r1, =0x11000000
ldr r2, =0x00800000
syscall IOS_Write

// set YUV yellow
mov r3, #0x0d800000
ldr r0, =((0 << 24) | (148 << 16) | (225 << 8)) | 1
str r0, [r3, #0x24]

mov r0, r4
syscall IOS_Close

mov r3, #0x0d800000
ldr r0, =0x101
str r0, [r3, #0x24]

loop:
b loop

file_str:
.string "/title/00010001/53415030/data/mem1.bin"
file_mem2:str
.string "/title/00010001/53415030/data/mem2.bin"