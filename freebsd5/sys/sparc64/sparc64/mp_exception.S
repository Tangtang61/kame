/*-
 * Copyright (c) 2002 Jake Burkholder.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/sparc64/sparc64/mp_exception.S,v 1.7 2002/07/26 15:54:04 jake Exp $
 */

#include <machine/asi.h>
#include <machine/ktr.h>
#include <machine/asmacros.h>
#include <machine/pstate.h>

#include "assym.s"

	.register	%g2, #ignore
	.register	%g3, #ignore

#define	IPI_WAIT(r1, r2, r3, r4) \
	lduw	[PCPU(CPUMASK)], r4 ;  \
	ATOMIC_CLEAR_INT(r1, r2, r3, r4) ; \
9:	lduw	[r1], r2 ; \
	brnz,a,pn r2, 9b ; \
	 nop

/*
 * Invalidate a phsyical page in the data cache.
 */
ENTRY(tl_ipi_dcache_page_inval)
#if KTR_COMPILE & KTR_SMP
	CATR(KTR_SMP, "ipi_dcache_page_inval: pa=%#lx"
	    , %g1, %g2, %g3, 7, 8, 9)
	ldx	[%g5 + ICA_PA], %g2
	stx	%g2, [%g1 + KTR_PARM1]
9:
#endif

	ldx	[%g5 + ICA_PA], %g6
	srlx	%g6, PAGE_SHIFT - DC_TAG_SHIFT, %g6

	SET(cache, %g3, %g2)
	lduw	[%g2 + DC_SIZE], %g3
	lduw	[%g2 + DC_LINESIZE], %g4
	sub	%g3, %g4, %g2

1:	ldxa	[%g2] ASI_DCACHE_TAG, %g1
	srlx	%g1, DC_VALID_SHIFT, %g3
	andcc	%g3, DC_VALID_MASK, %g0
	bz,pt	%xcc, 2f
	 set	DC_TAG_MASK, %g3
	sllx	%g3, DC_TAG_SHIFT, %g3
	and	%g1, %g3, %g1
	cmp	%g1, %g6
	bne,a,pt %xcc, 2f
	 nop
	stxa	%g1, [%g2] ASI_DCACHE_TAG
	membar	#Sync

2:	brgz,pt	%g2, 1b
	 sub	%g2, %g4, %g2

	IPI_WAIT(%g5, %g1, %g2, %g3)
	retry
END(tl_ipi_dcache_page_inval)

/*
 * Invalidate a phsyical page in the instruction cache.
 */
ENTRY(tl_ipi_icache_page_inval)
#if KTR_COMPILE & KTR_SMP
	CATR(KTR_SMP, "ipi_icache_page_inval: pa=%#lx"
	    , %g1, %g2, %g3, 7, 8, 9)
	ldx	[%g5 + ICA_PA], %g2
	stx	%g2, [%g1 + KTR_PARM1]
9:
#endif

	ldx	[%g5 + ICA_PA], %g6
	srlx	%g6, PAGE_SHIFT - IC_TAG_SHIFT, %g6

	SET(cache, %g3, %g2)
	lduw	[%g2 + IC_SIZE], %g3
	lduw	[%g2 + IC_LINESIZE], %g4
	sub	%g3, %g4, %g2

1:	ldda	[%g2] ASI_ICACHE_TAG, %g0 /*, %g1 */
	srlx	%g1, IC_VALID_SHIFT, %g3
	andcc	%g3, IC_VALID_MASK, %g0
	bz,pt	%xcc, 2f
	 set	IC_TAG_MASK, %g3
	sllx	%g3, IC_TAG_SHIFT, %g3
	and	%g1, %g3, %g1
	cmp	%g1, %g6
	bne,a,pt %xcc, 2f
	 nop
	stxa	%g1, [%g2] ASI_ICACHE_TAG
	membar	#Sync

2:	brgz,pt	%g2, 1b
	 sub	%g2, %g4, %g2

	IPI_WAIT(%g5, %g1, %g2, %g3)
	retry
END(tl_ipi_icache_page_inval)

/*
 * Trigger a softint at the desired level.
 */
ENTRY(tl_ipi_level)
#if KTR_COMPILE & KTR_SMP
	CATR(KTR_SMP, "tl_ipi_level: cpuid=%d mid=%d d1=%#lx d2=%#lx"
	    , %g1, %g2, %g3, 7, 8, 9)
	lduw	[PCPU(CPUID)], %g2
	stx	%g2, [%g1 + KTR_PARM1]
	lduw	[PCPU(MID)], %g2
	stx	%g2, [%g1 + KTR_PARM2]
	stx	%g4, [%g1 + KTR_PARM3]
	stx	%g5, [%g1 + KTR_PARM4]
9:
#endif

	mov	1, %g1
	sllx	%g1, %g5, %g1
	wr	%g1, 0, %asr20
	retry
END(tl_ipi_level)

/*
 * Demap a page from the dtlb and/or itlb.
 */
ENTRY(tl_ipi_tlb_page_demap)
#if KTR_COMPILE & KTR_SMP
	CATR(KTR_SMP, "ipi_tlb_page_demap: pm=%p va=%#lx"
	    , %g1, %g2, %g3, 7, 8, 9)
	ldx	[%g5 + ITA_PMAP], %g2
	stx	%g2, [%g1 + KTR_PARM1]
	ldx	[%g5 + ITA_VA], %g2
	stx	%g2, [%g1 + KTR_PARM2]
9:
#endif

	ldx	[%g5 + ITA_PMAP], %g1

	SET(kernel_pmap_store, %g3, %g2)
	mov	TLB_DEMAP_NUCLEUS | TLB_DEMAP_PAGE, %g3

	cmp	%g1, %g2
	movne	%xcc, TLB_DEMAP_PRIMARY | TLB_DEMAP_PAGE, %g3

	ldx	[%g5 + ITA_VA], %g2
	or	%g2, %g3, %g2

	stxa	%g0, [%g2] ASI_DMMU_DEMAP
	stxa	%g0, [%g2] ASI_IMMU_DEMAP
	membar	#Sync

	IPI_WAIT(%g5, %g1, %g2, %g3)
	retry
END(tl_ipi_tlb_page_demap)

/*
 * Demap a range of pages from the dtlb and itlb.
 */
ENTRY(tl_ipi_tlb_range_demap)
#if KTR_COMPILE & KTR_SMP
	CATR(KTR_SMP, "ipi_tlb_range_demap: pm=%p start=%#lx end=%#lx"
	    , %g1, %g2, %g3, 7, 8, 9)
	ldx	[%g5 + ITA_PMAP], %g2
	stx	%g2, [%g1 + KTR_PARM1]
	ldx	[%g5 + ITA_START], %g2
	stx	%g2, [%g1 + KTR_PARM2]
	ldx	[%g5 + ITA_END], %g2
	stx	%g2, [%g1 + KTR_PARM3]
9:
#endif

	ldx	[%g5 + ITA_PMAP], %g1

	SET(kernel_pmap_store, %g3, %g2)
	mov	TLB_DEMAP_NUCLEUS | TLB_DEMAP_PAGE, %g3

	cmp	%g1, %g2
	movne	%xcc, TLB_DEMAP_PRIMARY | TLB_DEMAP_PAGE, %g3

	ldx	[%g5 + ITA_START], %g1
	ldx	[%g5 + ITA_END], %g2

	set	PAGE_SIZE, %g6

1:	or	%g1, %g3, %g4
	stxa	%g0, [%g4] ASI_DMMU_DEMAP
	stxa	%g0, [%g4] ASI_IMMU_DEMAP
	membar	#Sync

	add	%g1, %g6, %g1
	cmp	%g1, %g2
	blt,a,pt %xcc, 1b
	 nop

	IPI_WAIT(%g5, %g1, %g2, %g3)
	retry
END(tl_ipi_tlb_range_demap)

/*
 * Demap the primary context from the dtlb and itlb.
 */
ENTRY(tl_ipi_tlb_context_demap)
#if KTR_COMPILE & KTR_SMP
	CATR(KTR_SMP, "ipi_tlb_page_demap: pm=%p va=%#lx"
	    , %g1, %g2, %g3, 7, 8, 9)
	ldx	[%g5 + ITA_PMAP], %g2
	stx	%g2, [%g1 + KTR_PARM1]
	ldx	[%g5 + ITA_VA], %g2
	stx	%g2, [%g1 + KTR_PARM2]
9:
#endif

	mov	TLB_DEMAP_PRIMARY | TLB_DEMAP_CONTEXT, %g1
	stxa	%g0, [%g1] ASI_DMMU_DEMAP
	stxa	%g0, [%g1] ASI_IMMU_DEMAP
	membar	#Sync

	IPI_WAIT(%g5, %g1, %g2, %g3)
	retry
END(tl_ipi_tlb_context_demap)
