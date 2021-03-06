/*	$OpenBSD: mutex.S,v 1.3 2007/05/31 23:50:25 drahn Exp $	*/

/*
 * Copyright (c) 2007 Dale Rahn
 * Copyright (c) 2007 Mark Kettenis
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "assym.h"

#include <machine/asm.h>

/* XXX */
#define GET_CPUINFO(r)	mfsprg r,0

ENTRY(mtx_init)
	li	%r5,0
	stw	%r4,MTX_WANTIPL(%r3)
	stw	%r5,MTX_OLDCPL(%r3)
	stw	%r5,MTX_OWNER(%r3)
	blr


ENTRY(mtx_enter)
	stwu	%r1,-32(%r1)			# reserve stack
	mflr	%r0
	stw	%r0,36(%r1)			# save return address
.L_retry:
	GET_CPUINFO(%r4)
	lwz	%r5,MTX_WANTIPL(%r3)		# load new ipl
	lis	%r6,_C_LABEL(imask)@ha		# convert into cpl
	slwi	%r5,%r5,2
	addi	%r5,%r5,_C_LABEL(imask)@l
	lwzx	%r5,%r5,%r6
	lwz	%r7,CI_CPL(%r4)			# load current cpl
	or	%r6,%r5,%r7			# raise	cpl
	stw	%r6,CI_CPL(%r4)			# store new cpl
	li	%r5,MTX_OWNER			# load offset constant
	lwarx	%r6,%r5,%r3			# load reserve owner
	cmpwi	0,%r6,0				# test owner == 0
	beq+	0,.L_mutex_free			# if owner == 0 branch free
.L_mutex_locked:
#ifdef DIAGNOSTIC
	cmpl	0,%r4,%r6
	beq-	.L_mutex_selflocked
#endif
	stw	%r3,28(%r1)			# save mtx during lcsplx
	la	%r4,28(%r1)
	stwcx.	%r3,0,%r4			# unreserve owner
	mr	%r3,%r7				# move old cpl to arg0
	bl	_C_LABEL(lcsplx)		# call splx on old cpl
	lwz	%r3,28(%r1)
	b	.L_retry

.L_mutex_free:
	stwcx.	%r4,%r5,%r3			# old owner was 0 cond store
	bne-	.L_mutex_locked			# branch if reserve cancelled
	stw	%r7,MTX_OLDCPL(%r3)		# save old ipl
	lwz	%r0,36(%r1)			# load return address
	mtlr	%r0
	addi	%r1,%r1,32			# restore stack
	blr

#ifdef DIAGNOSTIC
.L_mutex_selflocked:
	mr	%r5, %r3
	lis	%r3,.L_paniclocked@ha
	la	%r3,.L_paniclocked@l(%r3)
	bl 	panic
.L_paniclocked:
	.string "mtx_enter: recursed %x %x\n"
#endif


ENTRY(mtx_leave)
#ifdef DIAGNOSTIC
	lwz	%r6,MTX_OWNER(%r3)
	cmpwi   0,%r6,0                         # test owner == 0

	beq-	.L_mutex_notlocked
#endif
	li	%r4,0
	lwz	%r5,MTX_OLDCPL(%r3)
	stw	%r4,MTX_OLDCPL(%r3)
	stw	%r4,MTX_OWNER(%r3)
	GET_CPUINFO(%r4)
	mr	%r3,%r5
	lwz	%r5,CI_CPL(%r4)
	cmpl	0,%r3,%r5
	beq	1f
	b	_C_LABEL(lcsplx)
1:
	blr

#ifdef DIAGNOSTIC
.L_mutex_notlocked:
	GET_CPUINFO(%r4)
	mr	%r5, %r3
	lis	%r3,.L_panicnotlocked@ha
	la	%r3,.L_panicnotlocked@l(%r3)
	bl 	panic
.L_panicnotlocked:
	.string "mtx_leave: not locked %x %x\n"
#endif
