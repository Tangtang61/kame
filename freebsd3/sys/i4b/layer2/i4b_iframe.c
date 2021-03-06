/*
 * Copyright (c) 1997, 1998 Hellmuth Michaelis. All rights reserved.
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
 *---------------------------------------------------------------------------
 *
 *	i4b_iframe.c - i frame handling routines
 *	------------------------------------------
 *
 *	$Id: i4b_iframe.c,v 1.1 1998/12/27 21:46:49 phk Exp $ 
 *
 *      last edit-date: [Sat Dec  5 18:26:16 1998]
 *
 *---------------------------------------------------------------------------*/

#ifdef __FreeBSD__
#include "i4bq921.h"
#else
#define	NI4BQ921	1
#endif
#if NI4BQ921 > 0

#include <sys/param.h>
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
#include <sys/ioccom.h>
#else
#include <sys/ioctl.h>
#endif
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <net/if.h>

#ifdef __FreeBSD__
#include <machine/i4b_debug.h>
#include <machine/i4b_ioctl.h>
#include <machine/i4b_trace.h>
#else
#include <i4b/i4b_debug.h>
#include <i4b/i4b_ioctl.h>
#include <i4b/i4b_trace.h>
#endif

#include <i4b/include/i4b_global.h>
#include <i4b/include/i4b_l1l2.h>
#include <i4b/include/i4b_l2l3.h>
#include <i4b/include/i4b_isdnq931.h>
#include <i4b/include/i4b_mbuf.h>

#include <i4b/layer2/i4b_l2.h>
#include <i4b/layer2/i4b_l2fsm.h>

/*---------------------------------------------------------------------------*
 *	process i frame
 *	implements the routine "I COMMAND" Q.921 03/93 pp 77
 *---------------------------------------------------------------------------*/
void
i4b_rxd_i_frame(int unit, struct mbuf *m)
{
	l2_softc_t *l2sc = &l2_softc[unit];
	u_char *ptr = m->m_data;
	int nr;
	int ns;
	int p;
	int x;
	
	if(!((l2sc->tei_valid == TEI_VALID) &&
	     (l2sc->tei == GETTEI(*(ptr+OFF_TEI)))))
	{
		i4b_Dfreembuf(m);
		return;
	}

	if((l2sc->Q921_state != ST_MULTIFR) && (l2sc->Q921_state != ST_TIMREC))
	{
		i4b_Dfreembuf(m);
		DBGL2(L2_I_ERR, "i4b_rxd_i_frame", ("ERROR, state != (MF || TR)!\n"));
		return;
	}

	x = SPLI4B();
	
	nr = GETINR(*(ptr + OFF_INR));
	ns = GETINS(*(ptr + OFF_INS));
	p = GETIP(*(ptr + OFF_INR));
	
	i4b_rxd_ack(l2sc, nr);		/* last packet ack */

	if(l2sc->own_busy)		/* own receiver busy ? */
	{	
		i4b_Dfreembuf(m);	/* yes, discard information */

		if(p == 1)		/* P bit == 1 ? */
		{
			i4b_tx_rnr_response(l2sc, p); /* yes, tx RNR */
			l2sc->ack_pend = 0;	/* clear ACK pending */
		}
	}
	else	/* own receiver ready */
	{
		if(ns == l2sc->vr)	/* expected sequence number ? */
		{
			M128INC(l2sc->vr);	/* yes, update */
			
			l2sc->rej_excpt = 0;	/* clr reject exception */

			m_adj(m, I_HDR_LEN);	/* strip i frame header */

			l2sc->iframe_sent = 0;	/* reset i acked already */

			DL_Data_Ind(l2sc->unit, m);	/* pass data up */

			if(!l2sc->iframe_sent)
			{
				i4b_tx_rr_response(l2sc, p); /* yes, tx RR */
				l2sc->ack_pend = 0;	/* clr ACK pending */
			}
		}
		else	/* ERROR, sequence number NOT expected */
		{
			i4b_Dfreembuf(m);	/* discard information */

			if(l2sc->rej_excpt == 1)  /* already exception ? */
			{
				if(p == 1)	/* immediate response ? */
				{
					i4b_tx_rr_response(l2sc, p); /* yes, tx RR */
					l2sc->ack_pend = 0; /* clr ack pend */
				}
			}
			else	/* not in exception cond */
			{
				l2sc->rej_excpt = 1;	/* set exception */
				i4b_tx_rej_response(l2sc, p);	/* tx REJ */
				l2sc->ack_pend = 0;	/* clr ack pending */
			}
		}
	}

	/* sequence number ranges as expected ? */
	
	if(i4b_l2_nr_ok(nr, l2sc->va, l2sc->vs))
	{
		if(l2sc->Q921_state == ST_TIMREC)
		{
			l2sc->va = nr;
			splx(x);
			return;
		}

		if(l2sc->peer_busy)	/* yes, other side busy ? */
		{
			l2sc->va = nr;	/* yes, update ack count */
		}
		else	/* other side ready */
		{
			if(nr == l2sc->vs)	/* count expected ? */
			{
				l2sc->va = nr;	/* update ack */
				i4b_T200_stop(l2sc);
				i4b_T203_restart(l2sc);
			}
			else
			{
				if(nr != l2sc->va)
				{
					l2sc->va = nr;
					i4b_T200_restart(l2sc);
				}
			}
		}
	}
	else
	{
		i4b_nr_error_recovery(l2sc);	/* sequence error */
		l2sc->Q921_state = ST_AW_EST; 
	}
	splx(x);	
}

/*---------------------------------------------------------------------------*
 *	internal I FRAME QUEUED UP routine (Q.921 03/93 p 61)
 *---------------------------------------------------------------------------*/
void
i4b_i_frame_queued_up(l2_softc_t *l2sc)
{
	int x;
	struct mbuf *m;
	u_char *ptr;

	x = SPLI4B();

	if(l2sc->peer_busy)
	{
		DBGL2(L2_I_MSG, "i4b_i_frame_queued_up", ("peer busy!\n"));
		i4b_print_l2var(l2sc);
		splx(x);
		return;
	}

	if(l2sc->vs == ((l2sc->va + MAX_K_VALUE) & 127))
	{
		DBGL2(L2_I_ERR, "i4b_i_frame_queued_up", ("V(S) == ((V(A) + k) & 127)!\n"));
		DBGL2(L2_I_ERR, "i4b_i_frame_queued_up", ("state = %s\n", i4b_print_l2state(l2sc)));
		i4b_print_l2var(l2sc);
		splx(x);
		return;
	}

	IF_DEQUEUE(&l2sc->i_queue, m);    /* fetch next frame to tx */

	if(!m)
	{
		DBGL2(L2_I_ERR, "i4b_i_frame_queued_up", ("ERROR, mbuf NULL after IF_DEQUEUE\n"));
		splx(x);
		return;
	}

	ptr = m->m_data;
	
	PUTSAPI(SAPI_CCP, CR_CMD_TO_NT, *(ptr + OFF_SAPI));
	PUTTEI(l2sc->tei, *(ptr + OFF_TEI));

	*(ptr + OFF_INS) = (l2sc->vs << 1) & 0xfe; /* bit 0 = 0 */
	*(ptr + OFF_INR) = (l2sc->vr << 1) & 0xfe; /* P bit = 0 */

	PH_Data_Req(l2sc->unit, m, MBUF_DONTFREE); /* free'd when ack'd ! */

	l2sc->iframe_sent = 1;		/* in case we ack an I frame with another I frame */
	
	if(l2sc->ua_num != UA_EMPTY)	/* failsafe */
	{
		DBGL2(L2_I_ERR, "i4b_i_frame_queued_up", ("ERROR, l2sc->ua_num: %d != UA_EMPTY\n", l2sc->ua_num));
		i4b_print_l2var(l2sc);
		i4b_Dfreembuf(l2sc->ua_frame);
	}

	l2sc->ua_frame = m;		/* save unacked frame */
	l2sc->ua_num = l2sc->vs;	/* save frame number */

	M128INC(l2sc->vs);
	
	l2sc->ack_pend = 0;

	splx(x);

	if(l2sc->T200 == TIMER_IDLE)
	{
		i4b_T203_stop(l2sc);
		i4b_T200_start(l2sc);
	}
}
 
#endif /* NI4BQ921 > 0 */
