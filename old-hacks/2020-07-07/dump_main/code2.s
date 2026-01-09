	.file	"code.c"
	.machine ppc
	.section	".text"
	.section	.rodata.str1.4,"aMS",@progbits,1
	.align 2
.LC0:
	.string	"w"
	.align 2
.LC1:
	.string	"sda:/ctgpdump.bin"
	.section	".text"
	.align 2
	.globl fopen_hook
	.type	fopen_hook, @function
fopen_hook:
.LFB3:
	.cfi_startproc
	lis %r9,called@ha
	lwz %r10,called@l(%r9)
	cmpwi %cr0,%r10,0
	bne- %cr0,.L5
	stwu %r1,-16(%r1)
	.cfi_def_cfa_offset 16
	mflr %r0
	li %r10,1
	stw %r30,8(%r1)
	.cfi_register 65, 0
	.cfi_offset 30, -8
	mr %r30,%r4
	stw %r31,12(%r1)
	lis %r4,.LC0@ha
	.cfi_offset 31, -4
	mr %r31,%r3
	lis %r3,.LC1@ha
	la %r4,.LC0@l(%r4)
	la %r3,.LC1@l(%r3)
	stw %r0,20(%r1)
	.cfi_offset 65, 4
	stw %r10,called@l(%r9)
	bl call_original_fopen
	lis %r5,0x10
	lis %r4,0x9100
	ori %r5,%r5,0xcc0e
	bl call_fwrite
	lwz %r0,20(%r1)
	mr %r4,%r30
	mr %r3,%r31
	lwz %r30,8(%r1)
	mtlr %r0
	.cfi_restore 65
	lwz %r31,12(%r1)
	addi %r1,%r1,16
	.cfi_restore 31
	.cfi_restore 30
	.cfi_def_cfa_offset 0
.L5:
	b call_original_fopen
	.cfi_endproc
.LFE3:
	.size	fopen_hook, .-fopen_hook
	.align 2
	.globl search_fopen
	.type	search_fopen, @function
search_fopen:
.LFB0:
	.cfi_startproc
	lis %r8,0x7c60
	lis %r7,0x7c85
	lis %r6,0x7c04
	lis %r10,0x30
	mr %r9,%r3
	ori %r8,%r8,0x1b78
	ori %r7,%r7,0x2378
	ori %r6,%r6,0x378
	mtctr %r10
.L10:
	mr %r3,%r9
	lwzu %r10,4(%r9)
	cmpw %cr0,%r10,%r8
	beq- %cr0,.L18
.L8:
	bdnz .L10
.L19:
	li %r3,0
	blr
.L18:
	lwz %r10,8(%r9)
	cmpw %cr0,%r10,%r7
	bne+ %cr0,.L8
	lwz %r10,12(%r9)
	cmpw %cr0,%r10,%r6
	bne+ %cr0,.L8
	lbz %r10,16(%r9)
	cmplwi %cr0,%r10,75
	cmplwi %cr7,%r10,72
	beqlr- %cr0
	beqlr- %cr7
	bdnz .L10
	b .L19
	.cfi_endproc
.LFE0:
	.size	search_fopen, .-search_fopen
	.align 2
	.globl search_fwrite
	.type	search_fwrite, @function
search_fwrite:
.LFB1:
	.cfi_startproc
	lis %r8,0x7c68
	lis %r7,0x7c8a
	lis %r6,0x7ca9
	lis %r5,0x7cc7
	lis %r4,0x7d04
	lis %r10,0x30
	mr %r9,%r3
	ori %r8,%r8,0x1b78
	ori %r7,%r7,0x2378
	ori %r6,%r6,0x2b78
	ori %r5,%r5,0x3378
	ori %r4,%r4,0x4378
	mtctr %r10
.L23:
	mr %r3,%r9
	lwzu %r10,4(%r9)
	cmpw %cr0,%r10,%r8
	beq- %cr0,.L25
.L21:
	bdnz .L23
.L26:
	li %r3,0
	blr
.L25:
	lwz %r10,4(%r9)
	cmpw %cr0,%r10,%r7
	bne+ %cr0,.L21
	lwz %r10,8(%r9)
	cmpw %cr0,%r10,%r6
	bne+ %cr0,.L21
	lwz %r10,16(%r9)
	cmpw %cr0,%r10,%r5
	bne+ %cr0,.L21
	lwz %r10,20(%r9)
	cmpw %cr0,%r10,%r4
	beqlr- %cr0
	bdnz .L23
	b .L26
	.cfi_endproc
.LFE1:
	.size	search_fwrite, .-search_fwrite
	.align 2
	.globl setup
	.type	setup, @function
setup:
.LFB2:
	.cfi_startproc
	stwu %r1,-16(%r1)
	.cfi_def_cfa_offset 16
	stw %r31,12(%r1)
	.cfi_offset 31, -4
#APP
 # 76 "code.c" 1
	mfctr %r9

 # 0 "" 2
#NO_APP
	lis %r6,0x7c60
	lis %r3,0x7c85
	lis %r11,0x7c04
	lis %r10,0x30
	addis %r4,%r9,0xc0
	mr %r8,%r9
	ori %r6,%r6,0x1b78
	ori %r3,%r3,0x2378
	ori %r11,%r11,0x378
	mtctr %r10
.L31:
	lwz %r7,4(%r8)
	mr %r10,%r8
	addi %r8,%r8,4
	cmpw %cr0,%r7,%r6
	beq- %cr0,.L40
.L28:
	bdnz .L31
	li %r10,0
	li %r12,0
	lwz %r7,4(%r10)
.L30:
	lis %r8,fopen@ha
	lis %r6,0x7c68
	lis %r3,0x7c8a
	lis %r11,0x7ca9
	lis %r0,0x7cc7
	lis %r31,0x7d04
	stw %r10,fopen@l(%r8)
	ori %r6,%r6,0x1b78
	ori %r3,%r3,0x2378
	ori %r11,%r11,0x2b78
	ori %r0,%r0,0x3378
	ori %r31,%r31,0x4378
.L34:
	mr %r5,%r9
	lwzu %r8,4(%r9)
	cmpw %cr0,%r8,%r6
	cmplw %cr7,%r9,%r4
	beq- %cr0,.L41
.L32:
	bne+ %cr7,.L34
.L42:
	li %r5,0
.L33:
	lis %r9,fopen_hook@ha
	lis %r6,fwrite@ha
	la %r9,fopen_hook@l(%r9)
	lis %r8,fopen_thing_ha@ha
	cmplw %cr0,%r12,%r9
	subf %r12,%r12,%r9
	stw %r5,fwrite@l(%r6)
	xoris %r9,%r12,0xb400
	sth %r7,fopen_thing_ha@l(%r8)
	bge- %cr0,.L36
	oris %r9,%r12,0x4800
.L36:
	stw %r9,0(%r10)
#APP
 # 92 "code.c" 1
	mtctr %r9
 bctrl

 # 0 "" 2
#NO_APP
	lwz %r31,12(%r1)
	addi %r1,%r1,16
	.cfi_remember_state
	.cfi_restore 31
	.cfi_def_cfa_offset 0
	blr
.L40:
	.cfi_restore_state
	lwz %r5,8(%r8)
	cmpw %cr0,%r5,%r3
	bne+ %cr0,.L28
	lwz %r5,12(%r8)
	cmpw %cr0,%r5,%r11
	bne+ %cr0,.L28
	lbz %r5,16(%r8)
	cmplwi %cr0,%r5,75
	cmplwi %cr7,%r5,72
	beq- %cr0,.L29
	bne+ %cr7,.L28
.L29:
	mr %r12,%r10
	b .L30
.L41:
	lwz %r8,4(%r9)
	cmpw %cr0,%r8,%r3
	bne+ %cr0,.L32
	lwz %r8,8(%r9)
	cmpw %cr0,%r8,%r11
	bne+ %cr0,.L32
	lwz %r8,16(%r9)
	cmpw %cr0,%r8,%r0
	bne+ %cr0,.L32
	lwz %r8,20(%r9)
	cmpw %cr0,%r8,%r31
	beq- %cr0,.L33
	bne+ %cr7,.L34
	b .L42
	.cfi_endproc
.LFE2:
	.size	setup, .-setup
	.globl called
	.globl fopen_thing_ha
	.globl fwrite
	.globl fopen
	.section	.sbss,"aw",@nobits
	.align 2
	.type	called, @object
	.size	called, 4
called:
	.zero	4
	.section	.sdata,"aw"
	.align 2
	.type	fopen_thing_ha, @object
	.size	fopen_thing_ha, 2
fopen_thing_ha:
	.short	1
	.zero	2
	.type	fwrite, @object
	.size	fwrite, 4
fwrite:
	.long	1
	.type	fopen, @object
	.size	fopen, 4
fopen:
	.long	1
	.ident	"GCC: (devkitPPC release 37) 10.1.0"
