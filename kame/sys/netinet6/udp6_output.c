/*	$KAME: udp6_output.c,v 1.85 2005/06/16 19:58:07 jinmei Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)udp_var.h	8.1 (Berkeley) 6/10/93
 */

#if defined(__FreeBSD__) || defined (__NetBSD__)
#include "opt_ipsec.h"
#include "opt_inet.h"
#endif

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#ifdef __FreeBSD__
#include <sys/sysctl.h>
#endif
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/syslog.h>

#include <net/if.h>
#ifdef __FreeBSD__
#include <net/pfil.h>
#endif
#include <net/route.h>
#include <net/if_types.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/in_pcb.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#ifndef __OpenBSD__
#include <netinet6/in6_pcb.h>
#include <netinet6/udp6_var.h>
#endif
#include <netinet/icmp6.h>
#include <netinet6/ip6protosw.h>
#include <netinet6/scope6_var.h>

#ifdef __OpenBSD__
#undef IPSEC
#else
#ifdef IPSEC
#include <netinet6/ipsec.h>
#endif /* IPSEC */
#endif

#include <net/net_osdep.h>

/*
 * UDP protocol implementation.
 * Per RFC 768, August, 1980.
 */

#ifdef HAVE_NRL_INPCB
#define in6pcb		inpcb

#define udp6stat	udpstat
#define udp6s_opackets	udps_opackets
#endif
#ifdef __FreeBSD__
#define in6pcb		inpcb
#define udp6stat	udpstat
#define udp6s_opackets	udps_opackets
#endif

#ifdef __FreeBSD__
int
udp6_output(in6p, m, addr6, control, p)
	struct in6pcb *in6p;
	struct mbuf *m;
	struct mbuf *control;
	struct sockaddr *addr6;
	struct thread *p;
#elif defined(__NetBSD__)
int
udp6_output(in6p, m, addr6, control, p)
	struct in6pcb *in6p;
	struct mbuf *m;
	struct mbuf *addr6, *control;
	struct proc *p;
#else
int
udp6_output(in6p, m, addr6, control)
	struct in6pcb *in6p;
	struct mbuf *m;
	struct mbuf *addr6, *control;
#endif
{
	u_int32_t plen = sizeof(struct udphdr) + m->m_pkthdr.len;
	struct ip6_hdr *ip6;
	struct udphdr *udp6;
	struct in6_addr *laddr6 = NULL, *faddr6 = NULL;
	struct sockaddr_in6 *fsa6 = NULL;
	struct ifnet *oifp = NULL;
	int scope_ambiguous = 0;
#ifndef __FreeBSD__ 
	struct sockaddr_in6 lsa6_mapped; /* XXX ugly */
#endif
	u_int16_t fport;
	int error = 0;
	struct ip6_pktopts *optp, opt;
	int priv;
	int af = AF_INET6, hlen = sizeof(struct ip6_hdr);
#ifdef INET
#if defined(__NetBSD__)
	struct ip *ip;
	struct udpiphdr *ui;
#endif
#endif
	int flags = 0;
	struct sockaddr_in6 tmp;
#if defined(__OpenBSD__)
	struct proc *p = curproc;	/* XXX */
#endif

	priv = 0;
#if defined(__NetBSD__)
	if (p && !suser(p->p_ucred, &p->p_acflag))
		priv = 1;
#elif defined(__FreeBSD__)
	if (p && !suser(p))
		priv = 1;
#else
	if ((in6p->in6p_socket->so_state & SS_PRIV) != 0)
		priv = 1;
#endif

	if (addr6) {
#ifdef __FreeBSD__
		/* addr6 has been validated in udp6_send(). */
		fsa6 = (struct sockaddr_in6 *)addr6;
#else
		fsa6 = mtod(addr6, struct sockaddr_in6 *);

		if (addr6->m_len != sizeof(*fsa6))
			return (EINVAL);

		if (fsa6->sin6_family != AF_INET6)
			return (EAFNOSUPPORT);
#endif

		/* protect *sin6 from overwrites */
		tmp = *fsa6;
		fsa6 = &tmp;

		/*
		 * Application should provide a proper zone ID or the use of
		 * default zone IDs should be enabled.  Unfortunately, some
		 * applications do not behave as it should, so we need a
		 * workaround.  Even if an appropriate ID is not determined,
		 * we'll see if we can determine the outgoing interface.  If we
		 * can, determine the zone ID based on the interface below.
		 */
		if (fsa6->sin6_scope_id == 0 && !ip6_use_defzone)
			scope_ambiguous = 1;
		if ((error = sa6_embedscope(fsa6, ip6_use_defzone)) != 0)
			return (error);
	}

	if (control) {
		if ((error = ip6_setpktopts(control, &opt,
		    in6p->in6p_outputopts, priv, IPPROTO_UDP)) != 0)
			goto release;
		optp = &opt;
	} else
		optp = in6p->in6p_outputopts;
		

	if (fsa6) {
		faddr6 = &fsa6->sin6_addr;

		/*
		 * IPv4 version of udp_output calls in_pcbconnect in this case,
		 * which needs splnet and affects performance.
		 * Since we saw no essential reason for calling in_pcbconnect,
		 * we get rid of such kind of logic, and call in6_selectsrc
		 * and in6_pcbsetport in order to fill in the local address
		 * and the local port.
		 */
		if (fsa6->sin6_port == 0) {
			error = EADDRNOTAVAIL;
			goto release;
		}

		if (!IN6_IS_ADDR_UNSPECIFIED(&in6p->in6p_faddr)) {
			/* how about ::ffff:0.0.0.0 case? */
			error = EISCONN;
			goto release;
		}

		fport = fsa6->sin6_port; /* allow 0 port */

		if (IN6_IS_ADDR_V4MAPPED(faddr6)) {
#ifdef __OpenBSD__		/* does not support mapped addresses */
			if (1)
#else
			if ((in6p->in6p_flags & IN6P_IPV6_V6ONLY))
#endif
			{
				/*
				 * I believe we should explicitly discard the
				 * packet when mapped addresses are disabled,
				 * rather than send the packet as an IPv6 one.
				 * If we chose the latter approach, the packet
				 * might be sent out on the wire based on the
				 * default route, the situation which we'd
				 * probably want to avoid.
				 * (20010421 jinmei@kame.net)
				 */
				error = EINVAL;
				goto release;
			}
			if (!IN6_IS_ADDR_UNSPECIFIED(&in6p->in6p_laddr) &&
			    !IN6_IS_ADDR_V4MAPPED(&in6p->in6p_laddr)) {
				/*
				 * when remote addr is an IPv4-mapped address,
				 * local addr should not be an IPv6 address,
				 * since you cannot determine how to map IPv6
				 * source address to IPv4.
				 */
				error = EINVAL;
				goto release;
			}

			af = AF_INET;
		}

		if (!IN6_IS_ADDR_V4MAPPED(faddr6)) {
			laddr6 = in6_selectsrc(fsa6, optp,
			    in6p->in6p_moptions, &in6p->in6p_route,
			    &in6p->in6p_laddr, &oifp, &error);
			if (oifp && scope_ambiguous &&
			    (error = in6_setscope(&fsa6->sin6_addr,
			    oifp, NULL))) {
				goto release;
			}
		} else {
#ifndef __FreeBSD__
			/*
			 * XXX: freebsd[34] does not have in_selectsrc, but
			 * we can omit the whole part because freebsd4 calls
			 * udp_output() directly in this case, and thus we'll
			 * never see this path.
			 */
			if (IN6_IS_ADDR_UNSPECIFIED(&in6p->in6p_laddr)) {
				struct sockaddr_in *sinp, sin_dst;

				bzero(&sin_dst, sizeof(sin_dst));
				sin_dst.sin_family = AF_INET;
				sin_dst.sin_len = sizeof(sin_dst);
				bcopy(&fsa6->sin6_addr.s6_addr[12],
				      &sin_dst.sin_addr,
				      sizeof(sin_dst.sin_addr));
				sinp = in_selectsrc(&sin_dst,
						    (struct route *)&in6p->in6p_route,
						    in6p->in6p_socket->so_options,
						    NULL, &error);
				if (sinp == NULL) {
					if (error == 0)
						error = EADDRNOTAVAIL;
					goto release;
				}
				bzero(&lsa6_mapped, sizeof(lsa6_mapped));
				lsa6_mapped.sin6_family = AF_INET6;
				lsa6_mapped.sin6_len = sizeof(lsa6_mapped);
				/* ugly */
				lsa6_mapped.sin6_addr.s6_addr16[5] = 0xffff;
				bcopy(&sinp->sin_addr,
				      &lsa6_mapped.sin6_addr.s6_addr[12],
				      sizeof(sinp->sin_addr));
				laddr6 = &lsa6_mapped.sin6_addr;
			} else
#endif /* !freebsd */
			{
				laddr6 = &in6p->in6p_laddr;
			}
		}
		if (laddr6 == NULL) {
			if (error == 0)
				error = EADDRNOTAVAIL;
			goto release;
		}
		if (in6p->in6p_lport == 0 &&
		    (error = in6_pcbsetport(laddr6, in6p,
#ifdef __FreeBSD__
			p->td_ucred
#else
			p
#endif
		     )) != 0)
			goto release;
	} else {
		if (IN6_IS_ADDR_UNSPECIFIED(&in6p->in6p_faddr)) {
			error = ENOTCONN;
			goto release;
		}
		if (IN6_IS_ADDR_V4MAPPED(&in6p->in6p_faddr)) {
#ifdef __OpenBSD__		/* does not support mapped addresses */
			if (1)
#else
			if ((in6p->in6p_flags & IN6P_IPV6_V6ONLY))
#endif
			{
				/*
				 * XXX: this case would happen when the
				 * application sets the V6ONLY flag after
				 * connecting the foreign address.
				 * Such applications should be fixed,
				 * so we bark here.
				 */
				log(LOG_INFO, "udp6_output: IPV6_V6ONLY "
				    "option was set for a connected socket\n");
				error = EINVAL;
				goto release;
			} else
				af = AF_INET;
		}
		laddr6 = &in6p->in6p_laddr;
		faddr6 = &in6p->in6p_faddr;
		fport = in6p->in6p_fport;
	}

	if (af == AF_INET)
		hlen = sizeof(struct ip);

	/*
	 * Calculate data length and get a mbuf
	 * for UDP and IP6 headers.
	 */
	M_PREPEND(m, hlen + sizeof(struct udphdr), M_DONTWAIT);
	if (m == 0) {
		error = ENOBUFS;
		goto release;
	}

	/*
	 * Stuff checksum and output datagram.
	 */
	udp6 = (struct udphdr *)(mtod(m, caddr_t) + hlen);
	udp6->uh_sport = in6p->in6p_lport; /* lport is always set in the PCB */
	udp6->uh_dport = fport;
	if (plen <= 0xffff)
		udp6->uh_ulen = htons((u_int16_t)plen);
	else
		udp6->uh_ulen = 0;
	udp6->uh_sum = 0;

	switch (af) {
	case AF_INET6:
		ip6 = mtod(m, struct ip6_hdr *);
		ip6->ip6_flow	= in6p->in6p_flowinfo & IPV6_FLOWINFO_MASK;
		ip6->ip6_vfc 	&= ~IPV6_VERSION_MASK;
		ip6->ip6_vfc 	|= IPV6_VERSION;
#if 0				/* ip6_plen will be filled in ip6_output. */
		ip6->ip6_plen	= htons((u_int16_t)plen);
#endif
		ip6->ip6_nxt	= IPPROTO_UDP;
		ip6->ip6_hlim	= in6_selecthlim(in6p, oifp);
		ip6->ip6_src	= *laddr6;
		ip6->ip6_dst	= *faddr6;

		if ((udp6->uh_sum = in6_cksum(m, IPPROTO_UDP,
				sizeof(struct ip6_hdr), plen)) == 0) {
			udp6->uh_sum = 0xffff;
		}

		udp6stat.udp6s_opackets++;
#ifdef IPSEC
		if (ipsec_setsocket(m, in6p->in6p_socket) != 0) {
			error = ENOBUFS;
			goto release;
		}
#endif /* IPSEC */
#ifdef __FreeBSD__
		error = ip6_output(m, optp, &in6p->in6p_route,
		    flags, in6p->in6p_moptions, NULL, NULL);
#elif defined(__NetBSD__)
		error = ip6_output(m, optp, &in6p->in6p_route,
		    flags, in6p->in6p_moptions, in6p->in6p_socket, NULL);
#else
		error = ip6_output(m, optp, &in6p->in6p_route,
		    flags, in6p->in6p_moptions, NULL);
#endif

		break;
	case AF_INET:
#if defined(INET) && defined(__NetBSD__)
		/* can't transmit jumbogram over IPv4 */
		if (plen > 0xffff) {
			error = EMSGSIZE;
			goto release;
		}

		ip = mtod(m, struct ip *);
		ui = (struct udpiphdr *)ip;
		bzero(ui->ui_x1, sizeof ui->ui_x1);
		ui->ui_pr = IPPROTO_UDP;
		ui->ui_len = htons(plen);
		bcopy(&laddr6->s6_addr[12], &ui->ui_src, sizeof(ui->ui_src));
		ui->ui_ulen = ui->ui_len;

#ifdef  __NetBSD__
		flags = (in6p->in6p_socket->so_options &
			 (SO_DONTROUTE | SO_BROADCAST));
		bcopy(&fsa6->sin6_addr.s6_addr[12],
		      &ui->ui_dst, sizeof(ui->ui_dst));
		udp6->uh_sum = in_cksum(m, hlen + plen);
#endif
		if (udp6->uh_sum == 0)
			udp6->uh_sum = 0xffff;

		ip->ip_len = hlen + plen;
		ip->ip_ttl = in6_selecthlim(in6p, NULL); /* XXX */
		ip->ip_tos = 0;	/* XXX */

		ip->ip_len = hlen + plen; /* XXX */

		udpstat.udps_opackets++;
#ifdef IPSEC
		(void)ipsec_setsocket(m, NULL);	/* XXX */
#endif /* IPSEC */
#ifdef __NetBSD__
		error = ip_output(m, NULL, &in6p->in6p_route, flags /* XXX */);
#endif
		break;
#else
		error = EAFNOSUPPORT;
		goto release;
#endif
	}
	goto releaseopt;

release:
	m_freem(m);

releaseopt:
	if (control) {
		ip6_clearpktopts(&opt, -1);
		m_freem(control);
	}
	return (error);
}
