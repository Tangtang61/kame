|
|	execute NetBSD kernel
|
|	written by Yasha (ITOH Yasufumi)
|	public domain
|
|	$NetBSD: execkern.S,v 1.1 1998/09/01 19:51:56 itohy Exp $

/* XXX this value is from <machine/exec_aout.h> */
#define __LDPGSZ	8192

#define MFP		0x00E88000	/* MFP */
#define MFP_IERA	(MFP+0x07)	/* (B) interrupt enable reg A */
#define MFP_IERB	(MFP+0x09)	/* (B) interrupt enable reg B */
#define MFP_RSR		(MFP+0x2B)	/* (B) USART receiver status reg */

#ifndef SRAM_MEMSZ
#define SRAM		0x00ED0000	/* SRAM start addr */
#define SRAM_MEMSZ	(SRAM + 8)	/* (L) size of main memory */
#endif

|	a3+0	kernel image top address (a.out header is excluded)
|	a3+4	load address
|	a3+8	text size
|	a3+12	data size
|	a3+16	bss size
|	a3+20	symbol size
|	a3+24	(reserved) (d5)
|	a3+28	bootdev (d6)
|	a3+32	boothowto (d7)
|	a3+36	entry address (absolute address)

#ifndef XK_NO_C_INTERFACE
	.text
	.even
	.globl	_exec_kernel
_exec_kernel:
	addql	#4,sp
	moveal	sp@+,a3			| struct execkern_arg *
#endif

	moveal	a3@+,a0			| image address
	moveal	a3@+,a1			| load address
	movel	a1,d3

	|
	| copy image
	|

	| copy text segment
	movel	a3@+,d0			| text size
	movel	d0,d1
	jbsr	copy

	| clear gap between text and data
	negl	d1
	andil	#__LDPGSZ-1,d1
	movel	d1,d0			| gap size between text and data
	jbsr	clear

	| copy data segment
	movel	a3@+,d0			| data size
	jbsr	copy

	| clear bss
	movel	a3@+,d0			| bss size
	jbsr	clear

	| copy symbol table
	movel	a3@+,d0			| symbol table size
	movel	d0,a1@+
	beqs	Lnotable
	jbsr	copy

	| copy string table size
	movel	a0@+,d0
	movel	d0,a1@+
	beqs	Lnotable

	| copy string table
	subql	#4,d0			| table size is already copied
	jbsr	copy

Lnotable:

	| stop MFP interrupts (for compatibility)
	clrb	MFP_IERA
	clrb	MFP_IERB
	clrb	MFP_RSR

	|
	| execute kernel
	| start(load_addr, mem_max, kernel_end)
	|
	movel	a1,d0
	addql	#3,d0
	andib	#0xFC,d0
	subl	d3,d0
	movel	d0,sp@-			| arg #3 (kernel size)
	movel	SRAM_MEMSZ,sp@-		| arg #2 (RAM size from SRAM)
	movel	d3,sp@-			| arg #1 (load address)

#if 0
	movel	a3@+,d5			| (reserved)
	movel	a3@+,d6			| boot device
	movel	a3@+,d7			| boot howto

	movel	a3@+,a0			| entry address
#else	/* optimized */
	moveml	a3@+,d5-d7/a0
#endif

	| clear unused registers
	moveq	#0,d0
	moveq	#0,d1
	moveq	#0,d2
	moveq	#0,d3
	moveq	#0,d4
	moveal	d0,a1
	moveal	d0,a2
	moveal	d0,a3
	moveal	d0,a4
	moveal	d0,a5
	moveal	d0,a6

	jsr	a0@			| execute NetBSD kernel
	| NOTREACHED

	| ??? returned from kernel -- issue software reset
	subal	a1,a1
	moveml	0x00ff0000,#0x0101	| get RESET vectors (d0: ssp, a0: pc)
	moveml	#0x0101,a1@		| put them at 0x00000000 (for Xellent)
	.long	0x4E7B9801		| movec	a1,vbr
	jmp	a0@			| go to reset address


|
| utility routines
|

| copy d0 bytes from higher (a0) to lower (a1) address
1:	moveb	a0@+,a1@+
copy:	subql	#1,d0
	bpls	1b
	rts

| clear d0 bytes at a1
| do nothing if d0 is zero
1:	clrb	a1@+
clear:	subql	#1,d0
	bpls	1b
	rts
