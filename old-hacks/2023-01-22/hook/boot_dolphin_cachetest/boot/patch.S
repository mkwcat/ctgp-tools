#define	r0 0
#define	r1 1
#define	sp 1
#define	r2 2
#define	toc 2
#define	r3 3
#define	r4 4
#define	r5 5
#define	r6 6
#define	r7 7
#define	r8 8
#define	r9 9
#define	r10 10
#define	r11 11
#define	r12 12
#define	r13 13
#define	r14 14
#define	r15 15
#define	r16 16
#define	r17 17
#define	r18 18
#define	r19 19
#define	r20 20
#define	r21 21
#define	r22 22
#define	r23 23
#define	r24 24
#define	r25 25
#define	r26 26
#define	r27 27
#define	r28 28
#define	r29 29
#define	r30 30
#define	r31 31

#if 0
// Patch Code for PAL
.global PatchCodePStart
PatchCodePStart:
    blr // return decodeASH

    // decodeASH address
    // lis r31, 0x80218DC0@h
    // ori r31, r31, 0x80218DC0@l
    // Or rather getting it this way is easier to port
    mflr r12
    bl 4
    mflr r11
    subi r11, r11, 12

    // Patch to remove the Fatal Error message
    lis r3, 0x8004C0A8@h
    ori r3, r3, 0x8004C0A8@l

    lis r4, (PatchP_CTCODEEntry - PatchCodePStart)@h
    ori r4, r4, (PatchP_CTCODEEntry - PatchCodePStart)@l
    add r4, r11, r4
    sub r4, r4, r3
    rlwinm r4, r4, 0, 0x03FFFFFC
    oris r4, r4, 0x4800

    stw r4, 0(r3)
    dcbf 0, r3
    sync
    icbi 0, r4
    isync

    mtlr r12
    blr

PatchP_CTCODEEntry:

.global PatchCodePEnd
PatchCodePEnd:
#endif

#define addr_memcpy 0x80005F34
#define addr_pal_DCFlushRange 0x801A162C
#define addr_pal_ICInvalidateRange 0x801A1710

.global BAD1_P_Patch
.global BAD1_P_PatchEnd
BAD1_P_Patch:
    mr r14, r3
    mr r15, r5
    mflr r16
    mfctr r17

    lis r6, addr_memcpy@h
    ori r6, r6, addr_memcpy@l
    mtctr r6
    bctrl

    mr r3, r14
    mr r4, r15
    lis r6, addr_pal_DCFlushRange@h
    ori r6, r6, addr_pal_DCFlushRange@l
    mtctr r6
    bctrl

    mr r3, r14
    mr r4, r15
    lis r6, addr_pal_ICInvalidateRange@h
    ori r6, r6, addr_pal_ICInvalidateRange@l
    mtctr r6
    bctrl

    mtlr r16
    mtctr r17
    mr r3, r14
    blr
BAD1_P_PatchEnd:
