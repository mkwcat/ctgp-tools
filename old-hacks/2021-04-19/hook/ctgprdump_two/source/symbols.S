#define _LANGUAGE_ASSEMBLY
#include "asm.h"

#if 0
.section ".init"

.globl _start
_start:
	lis r1, (_stack_space + 0x1000 - 16)@h
	ori r1, r1, (_stack_space + 0x1000 - 16)@l
	b main_func

.section ".sbss"

.globl _stack_space
_stack_space:
	.space 0x1000
.size _stack_space, . - _stack_space
#endif






.globl boot1_main
.set boot1_main, 0x80780000
.globl chan_main
.set chan_main, 0x80800000

.section ".text"
.globl chan_start_thunk
chan_start_thunk:
	mfctr 3
	lis   4, main_start_sym@ha
	stw   3, main_start_sym@l(4)
	b     chan_start

.globl ctgp_hook_patch
ctgp_hook_patch:
	addi r1, r1, 32

	//#save packet
	lis r4, 0x8178B584@ha
	lis r5, 0x7C60
	ori r5, r5, 0x1B78
	stwu r5, 0x8053F3EC@l(r4)
	dcbf r0, r4
	
	//#region ID changer
	lis r4, 0x804267E4@ha
	li r5, 2
	stw r5, 0x804267E4@l(r4)
	
	//#USER region patch
	lis r4, 0x8066108C@ha
	lis r5, 0x3800
	ori r5, r5, 0x0002
	stwu r5, 0x8066108C@l(r4)
	dcbf r0, r4
	
	//#mission mode button (to see if it worked)
	lis r4, 0x805F580C@ha
	lis r5, 0x3800
	ori r5, r5, 0x0005
	stwu r5, 0x805F580C@l(r4)
	dcbf r0, r4
	blr
.globl ctgp_hook_patch_size
ctgp_hook_patch_size:
	.long . - ctgp_hook_patch
	