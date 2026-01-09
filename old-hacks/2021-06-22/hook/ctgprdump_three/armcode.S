#define YUV_RED         ((84 << 24) | (255 << 16) | (76 << 8) | 1)
#define YUV_DARK_RED    ((106 << 24) | (192 << 16) | (38 << 8) | 1)
#define YUV_GREEN       ((43 << 24) | (21 << 16) | (149 << 8) | 1)
#define YUV_DARK_GREEN  ((85 << 24) | (74 << 16) | (75 << 8) | 1)
#define YUV_BLUE        ((255 << 24) | (107 << 16) | (29 << 8) | 1)
#define YUV_DARK_BLUE   ((192 << 24) | (117 << 16) | (14 << 8) | 1)
#define YUV_PINK        ((170 << 24) | (181 << 16) | (180 << 8) | 1)
#define YUV_PURPLE      ((170 << 24) | (181 << 16) | (52 << 8) | 1)
#define YUV_CYAN        ((149 << 24) | (64 << 16) | (89 << 8) | 1)
#define YUV_YELLOW      ((0 << 24) | (148 << 16) | (225 << 8) | 1)
#define YUV_DARK_YELLOW ((64 << 24) | (138 << 16) | (113 << 8) | 1)
#define YUV_WHITE       ((128 << 24) | (128 << 16) | (255 << 8) | 1)
#define YUV_GRAY        ((128 << 24) | (128 << 16) | (128 << 8) | 1)

#define syscall .word 0xE6000010 | 0x20 *

#define IOS_Open 0x1C
#define IOS_Write 0x1F
#define IOS_Close 0x1D

.arm
ldr r4, =0x0d800024
ldr r0, =YUV_RED
str r0, [r4]

ldr r0, =0x12000000
ldr r1, =0x602
syscall IOS_Open

//mov r1, #0
//str r0, [r1, #0x34]
cmp r0, #0
ldrlt r1, =YUV_GREEN
ldrge r1, =YUV_BLUE
str r1, [r4]
infi1:
blt infi1

mov r5, r0
mov r1, #0
ldr r2, =0x01800000
syscall IOS_Write

ldr r1, =YUV_YELLOW
str r1, [r4]

mov r0, r5
syscall IOS_Close

ldr r1, =YUV_WHITE
str r1, [r4]

infi2:
b infi2