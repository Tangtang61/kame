/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)fdreg.h	7.1 (Berkeley) 5/9/91
 *	$Id: fdreg.h,v 1.2 1996/09/10 09:37:53 asami Exp $
 */

/*
 * AT floppy controller registers and bitfields
 */

/* uses NEC765 controller */
#include <i386/isa/ic/nec765.h>

#ifdef PC98
/* registers */
#define	FDSTS	0	/* NEC 765 Main Status Register (R) */
#define	FDDATA	2	/* NEC 765 Data Register (R/W) */
#define	FDOUT	4	/* Digital Output Register (W) */
#define	FDO_RST		0x80	/*  FDC RESET */
#define	FDO_FRY		0x40	/*  force READY */
#define	FDO_AIE		0x20	/*  Attention Interrupt Enable */
#define	FDO_DD		0x20	/*  FDD Mode Exchange 0:1M 1:640K */
#define	FDO_DMAE	0x10	/*  enable floppy DMA */
#define	FDO_MTON	0x08	/*  MOTOR ON (when EMTON=1)*/
#define	FDO_TMSK	0x04	/*  TIMER MASK */
#define	FDO_TTRG	0x01	/*  TIMER TRIGER */

#define	FDIN	4	/* Digital Input Register (R) */
#define	FDI_TYP0	0x04	/* FDD #1/#2 TYPE */
#define	FDI_TYP1	0x08	/* FDD #3/#4 TYPE */
#define	FDI_RDY		0x10	/* Ready */
#define	FDI_DMACH	0x20	/* DMA Channel */
#define	FDI_FINT0	0x40	/* Interrupt */
#define	FDI_FINT1	0x80	/* Interrupt */

#define	FDP_EMTON	0x04	/*  enable MTON */
#define	FDP_FDDEXC	0x02	/*  FDD Mode Exchange 1:1M 0:640K */
#define	FDP_PORTEXC	0x01	/*  PORT Exchane 1:1M 0:640K */

#else
/* registers */
#define	FDOUT	2	/* Digital Output Register (W) */
#define	FDO_FDSEL	0x03	/*  floppy device select */
#define	FDO_FRST	0x04	/*  floppy controller reset */
#define	FDO_FDMAEN	0x08	/*  enable floppy DMA and Interrupt */
#define	FDO_MOEN0	0x10	/*  motor enable drive 0 */
#define	FDO_MOEN1	0x20	/*  motor enable drive 1 */
#define	FDO_MOEN2	0x40	/*  motor enable drive 2 */
#define	FDO_MOEN3	0x80	/*  motor enable drive 3 */

#define	FDSTS	4	/* NEC 765 Main Status Register (R) */
#define	FDDATA	5	/* NEC 765 Data Register (R/W) */
#define	FDCTL	7	/* Control Register (W) */

#ifndef FDC_500KBPS
#  define	FDC_500KBPS	0x00	/* 500KBPS MFM drive transfer rate */
#  define	FDC_300KBPS	0x01	/* 300KBPS MFM drive transfer rate */
#  define	FDC_250KBPS	0x02	/* 250KBPS MFM drive transfer rate */
#  define	FDC_125KBPS	0x03	/* 125KBPS FM drive transfer rate */
				/* for some controllers 1MPBS instead */
#endif /* FDC_500KBPS */

#define	FDIN	7	/* Digital Input Register (R) */
#define	FDI_DCHG	0x80	/* diskette has been changed */
				/* requires drive and motor being selected */
				/* is cleared by any step pulse to drive */
#endif
