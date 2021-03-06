/*	$NetBSD: biosmemps2.S,v 1.2 2003/02/05 21:50:25 dsl Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jaromir Dolecek.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/asm.h>

	.text

/* int getextmemp2(void buffer)
   call int 15 function 0xc7 - later PS/2s - RETURN MEMORY-MAP INFORMATION
   return: 0=OK, nonzero=error
   buffer: filled with memory-map table structure
*/
ENTRY(getextmemps2)
	.code32
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	push	%esi
	push	%edi

	movl	8(%ebp), %edx	# parameter

	call	_C_LABEL(prot_to_real)
	.code16

	# do int15, function 0xc0 call to discover if C7h is supported
	movb	$0xc0, %ah
	int	$0x15
	setc	%bl
	jc	out		# 0xc0 not supported if carry set

	# check feature byte 2, bit 4 to see if return memory map is supported
	movb	%es:6(%bx), %al
	andb	$0x10, %al
	jnz	getmem		# 0xc7 supported
	
	# set %bl to indicate failure, and exit
	movb	$2, %bl
	jmp	out

getmem:
	# move the parameter to right register
	movl	%edx, %esi
	andl	$0xf, %esi
	shrl	$4, %edx
	mov	%ds, %ax
	add	%dx, %ax
	mov	%ax, %ds

	# actually call int15, function 0xc7 now
	movb	$0xc7, %ah
	int	$0x15
	setc	%bl		# save carry

out:
	calll	_C_LABEL(real_to_prot)
	.code32

	xorl	%eax, %eax
	movb	%bl, %al	# return value in %ax

	pop	%edi
	pop	%esi
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%ebp
	ret
