/*	$KAME: nd6_nbr.c,v 1.133 2003/12/05 01:35:18 keiichi Exp $	*/

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

#if defined(__FreeBSD__) && __FreeBSD__ >= 3
#include "opt_inet.h"
#include "opt_inet6.h"
#include "opt_ipsec.h"
#include "opt_mip6.h"
#endif
#ifdef __NetBSD__
#include "opt_inet.h"
#include "opt_ipsec.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#if !(defined(__FreeBSD__) && __FreeBSD__ >= 3)
#include <sys/ioctl.h>
#endif
#include <sys/syslog.h>
#include <sys/queue.h>
#if defined(__NetBSD__) || (defined(__FreeBSD__) && __FreeBSD__ >= 3)
#include <sys/callout.h>
#elif defined(__OpenBSD__)
#include <sys/timeout.h>
#endif
#ifdef __OpenBSD__
#include <dev/rndvar.h>
#endif

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_dl.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet6/in6_var.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/nd6.h>
#include <netinet/icmp6.h>

#ifdef __OpenBSD__	/* don't confuse KAME ipsec with OpenBSD ipsec */
#undef IPSEC
#endif

#ifdef IPSEC
#include <netinet6/ipsec.h>
#endif

#ifdef MIP6
#include <net/if_hif.h>
#include <netinet6/mip6.h>
#include <netinet6/mip6_var.h>
#include <netinet6/mip6_cncore.h>
#ifdef MIP6_HOME_AGENT
#include <netinet6/mip6_hacore.h>
#endif /* MIP6_HOME_AGENT */
#ifdef MIP6_MOBILE_NODE
#include <netinet6/mip6_mncore.h>
#endif /* MIP6_MOBILE_NODE */
#endif /* MIP6 */

#include <net/net_osdep.h>

#define SDL(s) ((struct sockaddr_dl *)s)

struct dadq;
static struct dadq *nd6_dad_find __P((struct ifaddr *));
static void nd6_dad_starttimer __P((struct dadq *, int));
static void nd6_dad_stoptimer __P((struct dadq *));
static void nd6_dad_timer __P((struct ifaddr *));
static void nd6_dad_ns_output __P((struct dadq *, struct ifaddr *));
static void nd6_dad_ns_input __P((struct ifaddr *));
static void nd6_dad_na_input __P((struct ifaddr *));

static int dad_ignore_ns = 0;	/* ignore NS in DAD - specwise incorrect*/
static int dad_maxtry = 15;	/* max # of *tries* to transmit DAD packet */

/*
 * Input a Neighbor Solicitation Message.
 *
 * Based on RFC 2461
 * Based on RFC 2462 (duplicated address detection)
 */
void
nd6_ns_input(m, off, icmp6len)
	struct mbuf *m;
	int off, icmp6len;
{
	struct ifnet *ifp = m->m_pkthdr.rcvif;
	struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
	struct nd_neighbor_solicit *nd_ns;
	struct sockaddr_in6 saddr6, daddr6, taddr6;
	char *lladdr = NULL;
	struct ifaddr *ifa;
	int lladdrlen = 0;
	int anycast = 0, proxy = 0, tentative = 0;
	int router = ip6_forwarding;
	int tlladdr;
	union nd_opts ndopts;
	struct sockaddr_dl *proxydl = NULL;

#ifndef PULLDOWN_TEST
	IP6_EXTHDR_CHECK(m, off, icmp6len,);
	nd_ns = (struct nd_neighbor_solicit *)((caddr_t)ip6 + off);
#else
	IP6_EXTHDR_GET(nd_ns, struct nd_neighbor_solicit *, m, off, icmp6len);
	if (nd_ns == NULL) {
		icmp6stat.icp6s_tooshort++;
		return;
	}
#endif

	if (ip6_getpktaddrs(m, &saddr6, &daddr6))
		goto bad;	/* should be impossible */

	ip6 = mtod(m, struct ip6_hdr *); /* adjust pointer for safety */
	bzero(&taddr6, sizeof(taddr6));
	taddr6.sin6_family = AF_INET6;
	taddr6.sin6_len = sizeof(struct sockaddr_in6);
	taddr6.sin6_addr = nd_ns->nd_ns_target;
	if (in6_addr2zoneid(ifp, &taddr6.sin6_addr, &taddr6.sin6_scope_id))
		goto bad;	/* XXX: impossible */
	in6_embedscope(&taddr6.sin6_addr, &taddr6); /* XXX */

	if (ip6->ip6_hlim != 255) {
		nd6log((LOG_ERR,
		    "nd6_ns_input: invalid hlim (%d) from %s to %s on %s\n",
		    ip6->ip6_hlim, ip6_sprintf(&ip6->ip6_src),
		    ip6_sprintf(&ip6->ip6_dst), if_name(ifp)));
		goto bad;
	}

	if (SA6_IS_ADDR_UNSPECIFIED(&saddr6)) {
		/* dst has to be a solicited node multicast address. */
		if (daddr6.sin6_addr.s6_addr16[0] == IPV6_ADDR_INT16_MLL &&
		    daddr6.sin6_addr.s6_addr32[1] == 0 &&
		    daddr6.sin6_addr.s6_addr32[2] == IPV6_ADDR_INT32_ONE &&
		    daddr6.sin6_addr.s6_addr8[12] == 0xff) {
			; /* good */
		} else {
			nd6log((LOG_INFO, "nd6_ns_input: bad DAD packet "
			    "(wrong ip6 dst)\n"));
			goto bad;
		}
	}

	if (IN6_IS_ADDR_MULTICAST(&taddr6.sin6_addr)) {
		nd6log((LOG_INFO, "nd6_ns_input: bad NS target (multicast)\n"));
		goto bad;
	}

	icmp6len -= sizeof(*nd_ns);
	nd6_option_init(nd_ns + 1, icmp6len, &ndopts);
	if (nd6_options(&ndopts) < 0) {
		nd6log((LOG_INFO,
		    "nd6_ns_input: invalid ND option, ignored\n"));
		/* nd6_options have incremented stats */
		goto freeit;
	}

	if (ndopts.nd_opts_src_lladdr) {
		lladdr = (char *)(ndopts.nd_opts_src_lladdr + 1);
		lladdrlen = ndopts.nd_opts_src_lladdr->nd_opt_len << 3;
	}

	if (IN6_IS_ADDR_UNSPECIFIED(&ip6->ip6_src) && lladdr) {
		nd6log((LOG_INFO, "nd6_ns_input: bad DAD packet "
		    "(link-layer address option)\n"));
		goto bad;
	}

	/*
	 * Attaching target link-layer address to the NA?
	 * (RFC 2461 7.2.4)
	 *
	 * NS IP dst is unicast/anycast			MUST NOT add
	 * NS IP dst is solicited-node multicast	MUST add
	 *
	 * In implementation, we add target link-layer address by default.
	 * We do not add one in MUST NOT cases.
	 */
#if 0 /* too much! */
	ifa = (struct ifaddr *)in6ifa_ifpwithaddr(ifp, &daddr6);
	if (ifa && (((struct in6_ifaddr *)ifa)->ia6_flags & IN6_IFF_ANYCAST))
		tlladdr = 0;
	else
#endif
	if (!IN6_IS_ADDR_MULTICAST(&daddr6.sin6_addr))
		tlladdr = 0;
	else
		tlladdr = 1;

	/*
	 * Target address (taddr6) must be either:
	 * (1) Valid unicast/anycast address for my receiving interface,
	 * (2) Unicast address for which I'm offering proxy service, or
	 * (3) "tentative" address on which DAD is being performed.
	 */
	/* (1) and (3) check. */
	ifa = (struct ifaddr *)in6ifa_ifpwithaddr(ifp, &taddr6.sin6_addr);

	/* (2) check. */
	if (!ifa) {
		struct rtentry *rt;
		struct sockaddr_in6 tsin6;

		tsin6 = taddr6;
#ifndef SCOPEDROUTING
		tsin6.sin6_scope_id = 0; /* XXX */
#endif

		rt = rtalloc1((struct sockaddr *)&tsin6, 0
#ifdef __FreeBSD__
			      , 0
#endif /* __FreeBSD__ */
			      );
		if (rt && (rt->rt_flags & RTF_ANNOUNCE) != 0 &&
		    rt->rt_gateway->sa_family == AF_LINK) {
			/*
			 * proxy NDP for single entry
			 */
			ifa = (struct ifaddr *)in6ifa_ifpforlinklocal(ifp,
				IN6_IFF_NOTREADY|IN6_IFF_ANYCAST);
			if (ifa) {
				proxy = 1;
				proxydl = SDL(rt->rt_gateway);
				router = 0;	/* XXX */
			}
		}
		if (rt)
			rtfree(rt);
	}
#if defined(MIP6) && defined(MIP6_HOME_AGENT)
	if (!ifa) {
		ifa = mip6_dad_find(&taddr6.sin6_addr, ifp);
	}
#endif /* MIP6 && MIP6_HOME_AGENT */
	if (!ifa) {
		/*
		 * We've got an NS packet, and we don't have that adddress
		 * assigned for us.  We MUST silently ignore it.
		 * See RFC2461 7.2.3.
		 */
		goto freeit;
	}
	anycast = ((struct in6_ifaddr *)ifa)->ia6_flags & IN6_IFF_ANYCAST;
	tentative = ((struct in6_ifaddr *)ifa)->ia6_flags & IN6_IFF_TENTATIVE;
	if (((struct in6_ifaddr *)ifa)->ia6_flags & IN6_IFF_DUPLICATED)
		goto freeit;

	if (lladdr && ((ifp->if_addrlen + 2 + 7) & ~7) != lladdrlen) {
		nd6log((LOG_INFO, "nd6_ns_input: lladdrlen mismatch for %s "
		    "(if %d, NS packet %d)\n",
		    ip6_sprintf(&taddr6.sin6_addr),
		    ifp->if_addrlen, lladdrlen - 2));
		goto bad;
	}

	if (SA6_ARE_ADDR_EQUAL(&((struct in6_ifaddr *)ifa)->ia_addr,
	    &saddr6)) {
		nd6log((LOG_INFO, "nd6_ns_input: duplicate IP6 address %s\n",
		    ip6_sprintf(&saddr6.sin6_addr)));
		goto freeit;
	}

	/*
	 * We have neighbor solicitation packet, with target address equals to
	 * one of my tentative address.
	 *
	 * src addr	how to process?
	 * ---		---
	 * multicast	of course, invalid (rejected in ip6_input)
	 * unicast	somebody is doing address resolution -> ignore
	 * unspec	dup address detection
	 *
	 * The processing is defined in RFC 2462.
	 */
	if (tentative) {
		/*
		 * If source address is unspecified address, it is for
		 * duplicated address detection.
		 *
		 * If not, the packet is for addess resolution;
		 * silently ignore it.
		 */
		if (SA6_IS_ADDR_UNSPECIFIED(&saddr6))
			nd6_dad_ns_input(ifa);

		goto freeit;
	}

	/*
	 * If the source address is unspecified address, entries must not
	 * be created or updated.
	 * It looks that sender is performing DAD.  Output NA toward
	 * all-node multicast address, to tell the sender that I'm using
	 * the address.
	 * S bit ("solicited") must be zero.
	 */
	if (SA6_IS_ADDR_UNSPECIFIED(&saddr6)) {
		struct sockaddr_in6 sa6_all;

		bzero(&sa6_all, sizeof(sa6_all));
		sa6_all.sin6_family = AF_INET6;
		sa6_all.sin6_len = sizeof(struct sockaddr_in6);
		sa6_all.sin6_addr = in6addr_linklocal_allnodes;
		if (in6_addr2zoneid(ifp, &sa6_all.sin6_addr,
				    &sa6_all.sin6_scope_id)) {
			goto bad; /* XXX impossible */
		}
		in6_embedscope(&sa6_all.sin6_addr, &sa6_all);
		nd6_na_output(ifp, &sa6_all, &taddr6,
		    ((anycast || proxy || !tlladdr) ? 0 : ND_NA_FLAG_OVERRIDE) |
		    (router ? ND_NA_FLAG_ROUTER : 0),
		    tlladdr, (struct sockaddr *)proxydl);
		goto freeit;
	}

	nd6_cache_lladdr(ifp, &saddr6, lladdr, lladdrlen,
	    ND_NEIGHBOR_SOLICIT, 0);

	nd6_na_output(ifp, &saddr6, &taddr6,
	    ((anycast || proxy || !tlladdr) ? 0 : ND_NA_FLAG_OVERRIDE) |
	    (router ? ND_NA_FLAG_ROUTER : 0) | ND_NA_FLAG_SOLICITED,
	    tlladdr, (struct sockaddr *)proxydl);
 freeit:
	m_freem(m);
	return;

 bad:
	nd6log((LOG_ERR, "nd6_ns_input: src=%s\n",
	    ip6_sprintf(&saddr6.sin6_addr)));
	nd6log((LOG_ERR, "nd6_ns_input: dst=%s\n",
	    ip6_sprintf(&daddr6.sin6_addr)));
	nd6log((LOG_ERR, "nd6_ns_input: tgt=%s\n",
	    ip6_sprintf(&taddr6.sin6_addr)));
	icmp6stat.icp6s_badns++;
	m_freem(m);
}

/*
 * Output a Neighbor Solicitation Message. Caller specifies:
 *	- ICMP6 header source IP6 address
 *	- ND6 header target IP6 address
 *	- ND6 header source datalink address
 *
 * Based on RFC 2461
 * Based on RFC 2462 (duplicated address detection)
 */
void
nd6_ns_output(ifp, daddr0, taddr0, ln, dad)
	struct ifnet *ifp;
	const struct sockaddr_in6 *daddr0, *taddr0;
	struct llinfo_nd6 *ln;	/* for source address determination */
	int dad;	/* duplicated address detection */
{
	struct mbuf *m;
	struct ip6_hdr *ip6;
	struct nd_neighbor_solicit *nd_ns;
	struct sockaddr_in6 *daddr6, *taddr6, src_sa, dst_sa;
#ifndef SCOPEDROUTING
	struct sockaddr_in6 daddr6_storage, taddr6_storage;
#endif
	struct ip6_moptions im6o;
	int icmp6len;
	int maxlen;
	caddr_t mac;
#if defined(MIP6) && defined(MIP6_MOBILE_NODE)
	int unicast_ns = 0;
#endif /* MIP6 && MIP6_MOBILE_NODE */
#ifdef NEW_STRUCT_ROUTE
	struct route ro;
#else
	struct route_in6 ro;
#endif

	bzero(&ro, sizeof(ro));

#ifndef SCOPEDROUTING
	/*
	 * XXX: since the daddr and taddr may come from the routing table
	 * entries, sin6_scope_id fields may not be filled in this case.
	 */
	daddr6 = &daddr6_storage;
	taddr6 = &taddr6_storage;
	if (daddr0) {
		*daddr6 = *daddr0;
		if (in6_addr2zoneid(ifp, &daddr6->sin6_addr,
				    &daddr6->sin6_scope_id)) {
			/* XXX impossible */
			return;
		}
	} else
		daddr6 = NULL;
	*taddr6 = *taddr0;
	if (in6_addr2zoneid(ifp, &taddr6->sin6_addr, &taddr6->sin6_scope_id)) {
		/* XXX: impossible */
		return;
	}
#else
	daddr6 = daddr0;
	taddr6 = taddr0;
#endif

	if (IN6_IS_ADDR_MULTICAST(&taddr6->sin6_addr))
		return;

	/* estimate the size of message */
	maxlen = sizeof(*ip6) + sizeof(*nd_ns);
	maxlen += (sizeof(struct nd_opt_hdr) + ifp->if_addrlen + 7) & ~7;
#ifdef DIAGNOSTIC
	if (max_linkhdr + maxlen >= MCLBYTES) {
		printf("nd6_ns_output: max_linkhdr + maxlen >= MCLBYTES "
		    "(%d + %d > %d)\n", max_linkhdr, maxlen, MCLBYTES);
		panic("nd6_ns_output: insufficient MCLBYTES");
		/* NOTREACHED */
	}
#endif

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m && max_linkhdr + maxlen >= MHLEN) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_free(m);
			m = NULL;
		}
	}
	if (m == NULL)
		return;
	m->m_pkthdr.rcvif = NULL;

#if defined(MIP6) && defined(MIP6_MOBILE_NODE)
	if (MIP6_IS_MN && daddr6 == NULL && !dad) {
		struct hif_softc *sc;
		struct mip6_bu *mbu;

		/* 10.20. Returning Home */
		for (sc = LIST_FIRST(&hif_softc_list); sc;
		    sc = LIST_NEXT(sc, hif_entry)) {
			mbu = mip6_bu_list_find_withpaddr(&sc->hif_bu_list,
			    taddr6, NULL);
			if (mbu == NULL)
				continue;
			if ((mbu->mbu_flags & IP6MU_HOME) == 0)
				continue;
#if 0			/* XXX WAITD is CN's BU only? */
			if (mbu->mbu_fsm_state == MIP6_BU_FSM_STATE_WAITD)
#endif
			{
				/* unspecified source */
				dad = 1;
				if (ln && ND6_IS_LLINFO_PROBREACH(ln))
					unicast_ns = 1;
			}
			break;
		}
	}
	if (!unicast_ns)
#endif /* MIP6 && MIP6_MOBILE_NODE */
	if (daddr6 == NULL || IN6_IS_ADDR_MULTICAST(&daddr6->sin6_addr)) {
		m->m_flags |= M_MCAST;
		im6o.im6o_multicast_ifp = ifp;
		im6o.im6o_multicast_hlim = 255;
		im6o.im6o_multicast_loop = 0;
	}

	icmp6len = sizeof(*nd_ns);
	m->m_pkthdr.len = m->m_len = sizeof(*ip6) + icmp6len;
	m->m_data += max_linkhdr;	/* or MH_ALIGN() equivalent? */

	/* fill neighbor solicitation packet */
	ip6 = mtod(m, struct ip6_hdr *);
	ip6->ip6_flow = 0;
	ip6->ip6_vfc &= ~IPV6_VERSION_MASK;
	ip6->ip6_vfc |= IPV6_VERSION;
	/* ip6->ip6_plen will be set later */
	ip6->ip6_nxt = IPPROTO_ICMPV6;
	ip6->ip6_hlim = 255;
	/* determine the source and destination addresses */
	bzero(&src_sa, sizeof(src_sa));
	bzero(&dst_sa, sizeof(dst_sa));
	src_sa.sin6_family = dst_sa.sin6_family = AF_INET6;
	src_sa.sin6_len = dst_sa.sin6_len = sizeof(struct sockaddr_in6);
	if (daddr6)
		dst_sa = *daddr6;
#if defined(MIP6) && defined(MIP6_MOBILE_NODE)
	else if (unicast_ns)
		dst_sa = *taddr6;
#endif /* MIP6 && MIP6_MOBILE_NODE */
	else {
		dst_sa.sin6_addr.s6_addr16[0] = IPV6_ADDR_INT16_MLL;
		dst_sa.sin6_addr.s6_addr16[1] = 0;
		dst_sa.sin6_addr.s6_addr32[1] = 0;
		dst_sa.sin6_addr.s6_addr32[2] = IPV6_ADDR_INT32_ONE;
		dst_sa.sin6_addr.s6_addr32[3] = taddr6->sin6_addr.s6_addr32[3];
		dst_sa.sin6_addr.s6_addr8[12] = 0xff;
		if (in6_addr2zoneid(ifp, &dst_sa.sin6_addr,
				    &dst_sa.sin6_scope_id)) {
			goto bad; /* XXX */
		}
		in6_embedscope(&dst_sa.sin6_addr, &dst_sa); /* XXX */
	}
	ip6->ip6_dst = dst_sa.sin6_addr;
	if (!dad) {
		/*
		 * RFC2461 7.2.2:
		 * "If the source address of the packet prompting the
		 * solicitation is the same as one of the addresses assigned
		 * to the outgoing interface, that address SHOULD be placed
		 * in the IP Source Address of the outgoing solicitation.
		 * Otherwise, any one of the addresses assigned to the
		 * interface should be used."
		 *
		 * We use the source address for the prompting packet
		 * (saddr6), if:
		 * - saddr6 is given from the caller (by giving "ln"), and
		 * - saddr6 belongs to the outgoing interface.
		 * Otherwise, we perform the source address selection as usual.
		 */
		struct ip6_hdr *hip6;		/* hold ip6 */
		struct sockaddr_in6 hsrc0, *hsrc = NULL;

		if (ln && ln->ln_hold) {
			hip6 = mtod(ln->ln_hold, struct ip6_hdr *);
			if (ip6_getpktaddrs(ln->ln_hold, &hsrc0, NULL))
				goto bad; /* XXX: impossible */
			hsrc = &hsrc0;
		}
		if (hsrc && in6ifa_ifpwithaddr(ifp, &hsrc->sin6_addr))
			src_sa = *hsrc;
		else {
			struct sockaddr_in6 *src0;
			int error;
#ifdef MIP6
			struct ip6_pktopts opts;
#endif /* MIP6 */

#ifdef MIP6
			ip6_initpktopts(&opts);
			opts.ip6po_flags |= IP6PO_USECOA;
#endif /* MIP6 */
			bcopy(&dst_sa, &ro.ro_dst, sizeof(dst_sa));
			src0 = in6_selectsrc(&dst_sa,
#ifdef MIP6
					     &opts,
#else /* !MIP6 */
					     NULL,
#endif /* !MIP6 */
					     NULL, &ro, NULL, NULL, &error);
			if (src0 == NULL) {
				nd6log((LOG_DEBUG,
				    "nd6_ns_output: source can't be "
				    "determined: dst=%s, error=%d\n",
				    ip6_sprintf(&dst_sa.sin6_addr), error));
				goto bad;
			}
			src_sa = *src0;
#ifndef SCOPEDROUTING		/* XXX */
			if (in6_addr2zoneid(ifp, &src_sa.sin6_addr,
					    &src_sa.sin6_scope_id)) {
				/* XXX: impossible*/
				goto bad;
			}
#endif
		}
	} else {
		/*
		 * Source address for DAD packet must always be IPv6
		 * unspecified address. (0::0)
		 * We actually don't have to 0-clear the address (we did it
		 * above), but we do so here explicitly to make the intention
		 * clearer.
		 */
		bzero(&src_sa.sin6_addr, sizeof(src_sa.sin6_addr));
	}
	ip6->ip6_src = src_sa.sin6_addr;
	/* attach the full sockaddr_in6 addresses to the packet */
	if (!ip6_setpktaddrs(m, &src_sa, &dst_sa))
		goto bad;
	nd_ns = (struct nd_neighbor_solicit *)(ip6 + 1);
	nd_ns->nd_ns_type = ND_NEIGHBOR_SOLICIT;
	nd_ns->nd_ns_code = 0;
	nd_ns->nd_ns_reserved = 0;
	nd_ns->nd_ns_target = taddr6->sin6_addr;
	in6_clearscope(&nd_ns->nd_ns_target); /* XXX */

	/*
	 * Add source link-layer address option.
	 *
	 *				spec		implementation
	 *				---		---
	 * DAD packet			MUST NOT	do not add the option
	 * there's no link layer address:
	 *				impossible	do not add the option
	 * there's link layer address:
	 *	Multicast NS		MUST add one	add the option
	 *	Unicast NS		SHOULD add one	add the option
	 */
	if (!dad && (mac = nd6_ifptomac(ifp))) {
		int optlen = sizeof(struct nd_opt_hdr) + ifp->if_addrlen;
		struct nd_opt_hdr *nd_opt = (struct nd_opt_hdr *)(nd_ns + 1);
		/* 8 byte alignments... */
		optlen = (optlen + 7) & ~7;

		m->m_pkthdr.len += optlen;
		m->m_len += optlen;
		icmp6len += optlen;
		bzero((caddr_t)nd_opt, optlen);
		nd_opt->nd_opt_type = ND_OPT_SOURCE_LINKADDR;
		nd_opt->nd_opt_len = optlen >> 3;
		bcopy(mac, (caddr_t)(nd_opt + 1), ifp->if_addrlen);
	}

	ip6->ip6_plen = htons((u_int16_t)icmp6len);
	nd_ns->nd_ns_cksum = 0;
	nd_ns->nd_ns_cksum =
	    in6_cksum(m, IPPROTO_ICMPV6, sizeof(*ip6), icmp6len);

#ifdef IPSEC
	/* Don't lookup socket */
	(void)ipsec_setsocket(m, NULL);
#endif
	ip6_output(m, NULL, &ro, dad ? IPV6_UNSPECSRC : 0, &im6o, NULL
#if defined(__FreeBSD__) && __FreeBSD_version >= 480000
		   ,NULL
#endif
		  );
	icmp6_ifstat_inc(ifp, ifs6_out_msg);
	icmp6_ifstat_inc(ifp, ifs6_out_neighborsolicit);
	icmp6stat.icp6s_outhist[ND_NEIGHBOR_SOLICIT]++;

	if (ro.ro_rt) {		/* we don't cache this route. */
		RTFREE(ro.ro_rt);
	}
	return;

  bad:
	if (ro.ro_rt) {
		RTFREE(ro.ro_rt);
	}
	m_freem(m);
	return;
}

/*
 * Neighbor advertisement input handling.
 *
 * Based on RFC 2461
 * Based on RFC 2462 (duplicated address detection)
 *
 * the following items are not implemented yet:
 * - proxy advertisement delay rule (RFC2461 7.2.8, last paragraph, SHOULD)
 * - anycast advertisement delay rule (RFC2461 7.2.7, SHOULD)
 */
void
nd6_na_input(m, off, icmp6len)
	struct mbuf *m;
	int off, icmp6len;
{
	struct ifnet *ifp = m->m_pkthdr.rcvif;
	struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
	struct nd_neighbor_advert *nd_na;
	struct sockaddr_in6 saddr6, taddr6;
	int flags;
	int is_router;
	int is_solicited;
	int is_override;
	char *lladdr = NULL;
	int lladdrlen = 0;
	struct ifaddr *ifa;
	struct llinfo_nd6 *ln;
	struct rtentry *rt;
	struct sockaddr_dl *sdl;
	union nd_opts ndopts;

	if (ip6->ip6_hlim != 255) {
		nd6log((LOG_ERR,
		    "nd6_na_input: invalid hlim (%d) from %s to %s on %s\n",
		    ip6->ip6_hlim, ip6_sprintf(&ip6->ip6_src),
		    ip6_sprintf(&ip6->ip6_dst), if_name(ifp)));
		goto bad;
	}

#ifndef PULLDOWN_TEST
	IP6_EXTHDR_CHECK(m, off, icmp6len,);
	nd_na = (struct nd_neighbor_advert *)((caddr_t)ip6 + off);
#else
	IP6_EXTHDR_GET(nd_na, struct nd_neighbor_advert *, m, off, icmp6len);
	if (nd_na == NULL) {
		icmp6stat.icp6s_tooshort++;
		return;
	}
#endif

	if (ip6_getpktaddrs(m, &saddr6, NULL))
		goto bad;	/* should be impossible */

	flags = nd_na->nd_na_flags_reserved;
	is_router = ((flags & ND_NA_FLAG_ROUTER) != 0);
	is_solicited = ((flags & ND_NA_FLAG_SOLICITED) != 0);
	is_override = ((flags & ND_NA_FLAG_OVERRIDE) != 0);

	bzero(&taddr6, sizeof(taddr6));
	taddr6.sin6_family = AF_INET6;
	taddr6.sin6_len = sizeof(taddr6);
	taddr6.sin6_addr = nd_na->nd_na_target;
	if (in6_addr2zoneid(ifp, &taddr6.sin6_addr, &taddr6.sin6_scope_id))
		return;		/* XXX: impossible */
	if (in6_embedscope(&taddr6.sin6_addr, &taddr6))
		return;

	if (IN6_IS_ADDR_MULTICAST(&taddr6.sin6_addr)) {
		nd6log((LOG_ERR,
		    "nd6_na_input: invalid target address %s\n",
		    ip6_sprintf(&taddr6.sin6_addr)));
		goto bad;
	}
	if (is_solicited && IN6_IS_ADDR_MULTICAST(&ip6->ip6_dst)) {
		nd6log((LOG_ERR,
		    "nd6_na_input: a solicited adv is multicasted\n"));
		goto bad;
	}

	icmp6len -= sizeof(*nd_na);
	nd6_option_init(nd_na + 1, icmp6len, &ndopts);
	if (nd6_options(&ndopts) < 0) {
		nd6log((LOG_INFO,
		    "nd6_na_input: invalid ND option, ignored\n"));
		/* nd6_options have incremented stats */
		goto freeit;
	}

	if (ndopts.nd_opts_tgt_lladdr) {
		lladdr = (char *)(ndopts.nd_opts_tgt_lladdr + 1);
		lladdrlen = ndopts.nd_opts_tgt_lladdr->nd_opt_len << 3;
	}

	ifa = (struct ifaddr *)in6ifa_ifpwithaddr(ifp, &taddr6.sin6_addr);
#if defined(MIP6) && defined(MIP6_HOME_AGENT)
	if (!ifa) {
		ifa = mip6_dad_find(&taddr6.sin6_addr, ifp);
	}
#endif /* MIP6 && MIP6_HOME_AGENT */

	/*
	 * Target address matches one of my interface address.
	 *
	 * If my address is tentative, this means that there's somebody
	 * already using the same address as mine.  This indicates DAD failure.
	 * This is defined in RFC 2462.
	 *
	 * Otherwise, process as defined in RFC 2461.
	 */
	if (ifa
	 && (((struct in6_ifaddr *)ifa)->ia6_flags & IN6_IFF_TENTATIVE)) {
		nd6_dad_na_input(ifa);
		goto freeit;
	}

	/* Just for safety, maybe unnecessary. */
	if (ifa) {
		log(LOG_ERR,
		    "nd6_na_input: duplicate IP6 address %s\n",
		    ip6_sprintf(&taddr6.sin6_addr));
		goto freeit;
	}

	if (lladdr && ((ifp->if_addrlen + 2 + 7) & ~7) != lladdrlen) {
		nd6log((LOG_INFO, "nd6_na_input: lladdrlen mismatch for %s "
		    "(if %d, NA packet %d)\n", ip6_sprintf(&taddr6.sin6_addr),
		    ifp->if_addrlen, lladdrlen - 2));
		goto bad;
	}

	/*
	 * If no neighbor cache entry is found, NA SHOULD silently be
	 * discarded.
	 */
	rt = nd6_lookup(&taddr6, 0, ifp);
	if ((rt == NULL) ||
	   ((ln = (struct llinfo_nd6 *)rt->rt_llinfo) == NULL) ||
	   ((sdl = SDL(rt->rt_gateway)) == NULL))
		goto freeit;

	if (ln->ln_state == ND6_LLINFO_INCOMPLETE) {
		/*
		 * If the link-layer has address, and no lladdr option came,
		 * discard the packet.
		 */
		if (ifp->if_addrlen && !lladdr)
			goto freeit;

		/*
		 * Record link-layer address, and update the state.
		 */
		sdl->sdl_alen = ifp->if_addrlen;
		bcopy(lladdr, LLADDR(sdl), ifp->if_addrlen);
		if (is_solicited) {
			ln->ln_state = ND6_LLINFO_REACHABLE;
			ln->ln_byhint = 0;
			if (!ND6_LLINFO_PERMANENT(ln)) {
				nd6_llinfo_settimer(ln,
				    (long)ND_IFINFO(rt->rt_ifp)->reachable * hz);
			}
		} else {
			ln->ln_state = ND6_LLINFO_STALE;
			nd6_llinfo_settimer(ln, (long)nd6_gctimer * hz);
		}
		if ((ln->ln_router = is_router) != 0) {
			/*
			 * This means a router's state has changed from
			 * non-reachable to probably reachable, and might
			 * affect the status of associated prefixes..
			 */
			pfxlist_onlink_check();
		}
	} else {
		int llchange;

		/*
		 * Check if the link-layer address has changed or not.
		 */
		if (!lladdr)
			llchange = 0;
		else {
			if (sdl->sdl_alen) {
				if (bcmp(lladdr, LLADDR(sdl), ifp->if_addrlen))
					llchange = 1;
				else
					llchange = 0;
			} else
				llchange = 1;
		}

		/*
		 * This is VERY complex.  Look at it with care.
		 *
		 * override solicit lladdr llchange	action
		 *					(L: record lladdr)
		 *
		 *	0	0	n	--	(2c)
		 *	0	0	y	n	(2b) L
		 *	0	0	y	y	(1)    REACHABLE->STALE
		 *	0	1	n	--	(2c)   *->REACHABLE
		 *	0	1	y	n	(2b) L *->REACHABLE
		 *	0	1	y	y	(1)    REACHABLE->STALE
		 *	1	0	n	--	(2a)
		 *	1	0	y	n	(2a) L
		 *	1	0	y	y	(2a) L *->STALE
		 *	1	1	n	--	(2a)   *->REACHABLE
		 *	1	1	y	n	(2a) L *->REACHABLE
		 *	1	1	y	y	(2a) L *->REACHABLE
		 */
		if (!is_override && (lladdr && llchange)) {	   /* (1) */
			/*
			 * If state is REACHABLE, make it STALE.
			 * no other updates should be done.
			 */
			if (ln->ln_state == ND6_LLINFO_REACHABLE) {
				ln->ln_state = ND6_LLINFO_STALE;
				nd6_llinfo_settimer(ln, (long)nd6_gctimer * hz);
			}
			goto freeit;
		} else if (is_override				   /* (2a) */
			|| (!is_override && (lladdr && !llchange)) /* (2b) */
			|| !lladdr) {				   /* (2c) */
			/*
			 * Update link-local address, if any.
			 */
			if (lladdr) {
				sdl->sdl_alen = ifp->if_addrlen;
				bcopy(lladdr, LLADDR(sdl), ifp->if_addrlen);
			}

			/*
			 * If solicited, make the state REACHABLE.
			 * If not solicited and the link-layer address was
			 * changed, make it STALE.
			 */
			if (is_solicited) {
				ln->ln_state = ND6_LLINFO_REACHABLE;
				ln->ln_byhint = 0;
				if (!ND6_LLINFO_PERMANENT(ln)) {
					nd6_llinfo_settimer(ln,
					    (long)ND_IFINFO(ifp)->reachable * hz);
				}
			} else {
				if (lladdr && llchange) {
					ln->ln_state = ND6_LLINFO_STALE;
					nd6_llinfo_settimer(ln,
					    (long)nd6_gctimer * hz);
				}
			}
		}

		if (ln->ln_router && !is_router) {
			/*
			 * The peer dropped the router flag.
			 * Remove the sender from the Default Router List and
			 * update the Destination Cache entries.
			 */
			struct nd_defrouter *dr;
			int s;

			/*
			 * Lock to protect the default router list.
			 * XXX: this might be unnecessary, since this function
			 * is only called under the network software interrupt
			 * context.  However, we keep it just for safety.
			 */
#if defined(__NetBSD__) || defined(__OpenBSD__)
			s = splsoftnet();
#else
			s = splnet();
#endif
			dr = defrouter_lookup((struct sockaddr_in6 *)rt_key(rt),
			    rt->rt_ifp);
			if (dr)
				defrtrlist_del(dr);
			else if (!ip6_forwarding) {
				/*
				 * Even if the neighbor is not in the default
				 * router list, the neighbor may be used
				 * as a next hop for some destinations
				 * (e.g. redirect case). So we must
				 * call rt6_flush explicitly.
				 */
				rt6_flush(&saddr6, rt->rt_ifp);
			}
			splx(s);
		}
		ln->ln_router = is_router;
	}
	rt->rt_flags &= ~RTF_REJECT;
	ln->ln_asked = 0;
	if (ln->ln_hold) {
		/*
		 * we assume ifp is not a loopback here, so just set the 2nd
		 * argument as the 1st one.
		 */
#if defined(MIP6) && defined(MIP6_HOME_AGENT)
		struct mip6_bc *mbc;

		mbc = mip6_temp_deleted_proxy(ln->ln_hold);
#endif
		nd6_output(ifp, ifp, ln->ln_hold,
			   (struct sockaddr_in6 *)rt_key(rt), rt);
		ln->ln_hold = NULL;
#if defined(MIP6) && defined(MIP6_HOME_AGENT)
		/* restore the temporally deleted proxy nd entry 
		   to send binding ack. in some special cases */
		if (mbc)
			mip6_bc_proxy_control(&mbc->mbc_phaddr, &mbc->mbc_addr, RTM_ADD);
#endif
	}

 freeit:
	m_freem(m);
	return;

 bad:
	icmp6stat.icp6s_badna++;
	m_freem(m);
}

/*
 * Neighbor advertisement output handling.
 *
 * Based on RFC 2461
 *
 * the following items are not implemented yet:
 * - proxy advertisement delay rule (RFC2461 7.2.8, last paragraph, SHOULD)
 * - anycast advertisement delay rule (RFC2461 7.2.7, SHOULD)
 */
void
nd6_na_output(ifp, daddr6, taddr6, flags, tlladdr, sdl0)
	struct ifnet *ifp;
	const struct sockaddr_in6 *daddr6, *taddr6;
	u_long flags;
	int tlladdr;		/* 1 if include target link-layer address */
	struct sockaddr *sdl0;	/* sockaddr_dl (= proxy NA) or NULL */
{
	struct mbuf *m;
	struct ip6_hdr *ip6;
	struct nd_neighbor_advert *nd_na;
	struct ip6_moptions im6o;
	struct sockaddr_in6 src_sa, dst_sa, *src0;
	int icmp6len, maxlen, error;
	caddr_t mac;
#ifdef NEW_STRUCT_ROUTE
	struct route ro;
#else
	struct route_in6 ro;
#endif
#ifdef MIP6
	struct ip6_pktopts opts;
#endif /* MIP6 */

	mac = NULL;
	bzero(&ro, sizeof(ro));

	/* estimate the size of message */
	maxlen = sizeof(*ip6) + sizeof(*nd_na);
	maxlen += (sizeof(struct nd_opt_hdr) + ifp->if_addrlen + 7) & ~7;
#ifdef DIAGNOSTIC
	if (max_linkhdr + maxlen >= MCLBYTES) {
		printf("nd6_na_output: max_linkhdr + maxlen >= MCLBYTES "
		    "(%d + %d > %d)\n", max_linkhdr, maxlen, MCLBYTES);
		panic("nd6_na_output: insufficient MCLBYTES");
		/* NOTREACHED */
	}
#endif

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m && max_linkhdr + maxlen >= MHLEN) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_free(m);
			m = NULL;
		}
	}
	if (m == NULL)
		return;
	m->m_pkthdr.rcvif = NULL;

	if (IN6_IS_ADDR_MULTICAST(&daddr6->sin6_addr)) {
		m->m_flags |= M_MCAST;
		im6o.im6o_multicast_ifp = ifp;
		im6o.im6o_multicast_hlim = 255;
		im6o.im6o_multicast_loop = 0;
	}

	icmp6len = sizeof(*nd_na);
	m->m_pkthdr.len = m->m_len = sizeof(struct ip6_hdr) + icmp6len;
	m->m_data += max_linkhdr;	/* or MH_ALIGN() equivalent? */

	/* fill neighbor advertisement packet */
	ip6 = mtod(m, struct ip6_hdr *);
	ip6->ip6_flow = 0;
	ip6->ip6_vfc &= ~IPV6_VERSION_MASK;
	ip6->ip6_vfc |= IPV6_VERSION;
	ip6->ip6_nxt = IPPROTO_ICMPV6;
	ip6->ip6_hlim = 255;
	dst_sa = *daddr6;
	if (SA6_IS_ADDR_UNSPECIFIED(daddr6)) {
		/* reply to DAD */
		dst_sa.sin6_addr.s6_addr16[0] = IPV6_ADDR_INT16_MLL;
		dst_sa.sin6_addr.s6_addr16[1] = 0;
		dst_sa.sin6_addr.s6_addr32[1] = 0;
		dst_sa.sin6_addr.s6_addr32[2] = 0;
		dst_sa.sin6_addr.s6_addr32[3] = IPV6_ADDR_INT32_ONE;
		if (in6_addr2zoneid(ifp, &dst_sa.sin6_addr,
				    &dst_sa.sin6_scope_id)) {
			goto bad;
		}
		in6_embedscope(&dst_sa.sin6_addr, &dst_sa); /* XXX */

		flags &= ~ND_NA_FLAG_SOLICITED;
	}
	ip6->ip6_dst = dst_sa.sin6_addr;

	/*
	 * Select a source whose scope is the same as that of the dest.
	 */
#ifdef MIP6
	ip6_initpktopts(&opts);
	opts.ip6po_flags |= IP6PO_USECOA;
#endif /* MIP6 */
	bcopy(&dst_sa, &ro.ro_dst, sizeof(dst_sa));
	src0 = in6_selectsrc(&dst_sa,
#ifdef MIP6
			     &opts,
#else /* !MIP6 */
			     NULL,
#endif /* !MIP6 */
			     NULL, &ro, NULL, NULL, &error);
	if (src0 == NULL) {
		nd6log((LOG_DEBUG, "nd6_na_output: source can't be "
		    "determined: dst=%s, error=%d\n",
		    ip6_sprintf(&dst_sa.sin6_addr), error));
		goto bad;
	}
	src_sa = *src0;
#ifndef SCOPEDROUTING		/* XXX */
	if (in6_addr2zoneid(ifp, &src_sa.sin6_addr, &src_sa.sin6_scope_id))
		goto bad;	/* XXX: impossible*/
#endif
	ip6->ip6_src = src_sa.sin6_addr;
	/* attach the full sockaddr_in6 addresses to the packet */
	if (!ip6_setpktaddrs(m, &src_sa, &dst_sa))
		goto bad;
	nd_na = (struct nd_neighbor_advert *)(ip6 + 1);
	nd_na->nd_na_type = ND_NEIGHBOR_ADVERT;
	nd_na->nd_na_code = 0;
	nd_na->nd_na_target = taddr6->sin6_addr;
	in6_clearscope(&nd_na->nd_na_target); /* XXX */

	/*
	 * "tlladdr" indicates NS's condition for adding tlladdr or not.
	 * see nd6_ns_input() for details.
	 * Basically, if NS packet is sent to unicast/anycast addr,
	 * target lladdr option SHOULD NOT be included.
	 */
	if (tlladdr) {
		/*
		 * sdl0 != NULL indicates proxy NA.  If we do proxy, use
		 * lladdr in sdl0.  If we are not proxying (sending NA for
		 * my address) use lladdr configured for the interface.
		 */
		if (sdl0 == NULL)
			mac = nd6_ifptomac(ifp);
		else if (sdl0->sa_family == AF_LINK) {
			struct sockaddr_dl *sdl;
			sdl = (struct sockaddr_dl *)sdl0;
			if (sdl->sdl_alen == ifp->if_addrlen)
				mac = LLADDR(sdl);
		}
	}
	if (tlladdr && mac) {
		int optlen = sizeof(struct nd_opt_hdr) + ifp->if_addrlen;
		struct nd_opt_hdr *nd_opt = (struct nd_opt_hdr *)(nd_na + 1);

		/* roundup to 8 bytes alignment! */
		optlen = (optlen + 7) & ~7;

		m->m_pkthdr.len += optlen;
		m->m_len += optlen;
		icmp6len += optlen;
		bzero((caddr_t)nd_opt, optlen);
		nd_opt->nd_opt_type = ND_OPT_TARGET_LINKADDR;
		nd_opt->nd_opt_len = optlen >> 3;
		bcopy(mac, (caddr_t)(nd_opt + 1), ifp->if_addrlen);
	} else
		flags &= ~ND_NA_FLAG_OVERRIDE;

	ip6->ip6_plen = htons((u_int16_t)icmp6len);
	nd_na->nd_na_flags_reserved = flags;
	nd_na->nd_na_cksum = 0;
	nd_na->nd_na_cksum =
	    in6_cksum(m, IPPROTO_ICMPV6, sizeof(struct ip6_hdr), icmp6len);

#ifdef IPSEC
	/* Don't lookup socket */
	(void)ipsec_setsocket(m, NULL);
#endif
	ip6_output(m, NULL, &ro, 0, &im6o, NULL
#if defined(__FreeBSD__) && __FreeBSD_version >= 480000
		   ,NULL
#endif
		  );

	icmp6_ifstat_inc(ifp, ifs6_out_msg);
	icmp6_ifstat_inc(ifp, ifs6_out_neighboradvert);
	icmp6stat.icp6s_outhist[ND_NEIGHBOR_ADVERT]++;

	if (ro.ro_rt) {		/* we don't cache this route. */
		RTFREE(ro.ro_rt);
	}
	return;

  bad:
	if (ro.ro_rt) {
		RTFREE(ro.ro_rt);
	}
	m_freem(m);
	return;
}

caddr_t
nd6_ifptomac(ifp)
	struct ifnet *ifp;
{
	switch (ifp->if_type) {
	case IFT_ARCNET:
	case IFT_ETHER:
	case IFT_FDDI:
	case IFT_IEEE1394:
#ifdef IFT_PROPVIRTUAL
	case IFT_PROPVIRTUAL:
#endif
#ifdef IFT_L2VLAN
	case IFT_L2VLAN:
#endif
#ifdef IFT_IEEE80211
	case IFT_IEEE80211:
#endif
#ifdef __NetBSD__
		return LLADDR(ifp->if_sadl);
#else
		return ((caddr_t)(ifp + 1));
#endif
	default:
		return NULL;
	}
}

TAILQ_HEAD(dadq_head, dadq);
struct dadq {
	TAILQ_ENTRY(dadq) dad_list;
	struct ifaddr *dad_ifa;
	int dad_count;		/* max NS to send */
	int dad_ns_tcount;	/* # of trials to send NS */
	int dad_ns_ocount;	/* NS sent so far */
	int dad_ns_icount;
	int dad_na_icount;
#if defined(__NetBSD__) || (defined(__FreeBSD__) && __FreeBSD__ >= 3)
	struct callout dad_timer_ch;
#elif defined(__OpenBSD__)
	struct timeout dad_timer_ch;
#endif
};

static struct dadq_head dadq;
static int dad_init = 0;

static struct dadq *
nd6_dad_find(ifa)
	struct ifaddr *ifa;
{
	struct dadq *dp;

	for (dp = dadq.tqh_first; dp; dp = dp->dad_list.tqe_next) {
		if (dp->dad_ifa == ifa)
			return dp;
	}
	return NULL;
}

static void
nd6_dad_starttimer(dp, ticks)
	struct dadq *dp;
	int ticks;
{

#if defined(__NetBSD__) || (defined(__FreeBSD__) && __FreeBSD__ >= 3)
	callout_reset(&dp->dad_timer_ch, ticks,
	    (void (*) __P((void *)))nd6_dad_timer, (void *)dp->dad_ifa);
#elif defined(__OpenBSD__)
	timeout_set(&dp->dad_timer_ch, (void (*) __P((void *)))nd6_dad_timer,
	    (void *)dp->dad_ifa);
	timeout_add(&dp->dad_timer_ch, ticks);
#else
	timeout((void (*) __P((void *)))nd6_dad_timer, (void *)dp->dad_ifa,
	    ticks);
#endif
}

static void
nd6_dad_stoptimer(dp)
	struct dadq *dp;
{

#if defined(__NetBSD__) || (defined(__FreeBSD__) && __FreeBSD__ >= 3)
	callout_stop(&dp->dad_timer_ch);
#elif defined(__OpenBSD__)
	timeout_del(&dp->dad_timer_ch);
#else
	untimeout((void (*) __P((void *)))nd6_dad_timer, (void *)dp->dad_ifa);
#endif
}

/*
 * Start Duplicated Address Detection (DAD) for specified interface address.
 */
void
nd6_dad_start(ifa, tick)
	struct ifaddr *ifa;
	int *tick;	/* minimum delay ticks for IFF_UP event */
{
	struct in6_ifaddr *ia = (struct in6_ifaddr *)ifa;
	struct dadq *dp;

	if (!dad_init) {
		TAILQ_INIT(&dadq);
		dad_init++;
	}

	/*
	 * If we don't need DAD, don't do it.
	 * There are several cases:
	 * - DAD is disabled (ip6_dad_count == 0)
	 * - the interface address is anycast
	 */
	if (!(ia->ia6_flags & IN6_IFF_TENTATIVE)) {
		log(LOG_DEBUG,
			"nd6_dad_start: called with non-tentative address "
			"%s(%s)\n",
			ip6_sprintf(&ia->ia_addr.sin6_addr),
			ifa->ifa_ifp ? if_name(ifa->ifa_ifp) : "???");
		return;
	}
	if (ia->ia6_flags & IN6_IFF_ANYCAST) {
		ia->ia6_flags &= ~IN6_IFF_TENTATIVE;
		return;
	}
	if (!ip6_dad_count) {
		ia->ia6_flags &= ~IN6_IFF_TENTATIVE;
#if defined(MIP6) && defined(MIP6_HOME_AGENT)
		mip6_dad_success(ifa);
#endif /* MIP6 && MIP6_HOME_AGENT */
		return;
	}
	if (!ifa->ifa_ifp)
		panic("nd6_dad_start: ifa->ifa_ifp == NULL");
	if (!(ifa->ifa_ifp->if_flags & IFF_UP)) {
#if defined(MIP6) && defined(MIP6_HOME_AGENT)
		mip6_dad_error(ifa, IP6_MH_BAS_INSUFFICIENT);
#endif /* MIP6 && MIP6_HOME_AGENT */
		return;
	}
	if (nd6_dad_find(ifa) != NULL) {
		/* DAD already in progress */
#if defined(MIP6) && defined(MIP6_HOME_AGENT)
		mip6_dad_error(ifa, IP6_MH_BAS_INSUFFICIENT);
#endif /* MIP6 && MIP6_HOME_AGENT */
		return;
	}

	dp = malloc(sizeof(*dp), M_IP6NDP, M_NOWAIT);
	if (dp == NULL) {
		log(LOG_ERR, "nd6_dad_start: memory allocation failed for "
			"%s(%s)\n",
			ip6_sprintf(&ia->ia_addr.sin6_addr),
			ifa->ifa_ifp ? if_name(ifa->ifa_ifp) : "???");
#if defined(MIP6) && defined(MIP6_HOME_AGENT)
		mip6_dad_error(ifa, IP6MA_STATUS_RESOURCES);
#endif /* MIP6 && MIP6_HOME_AGENT */
		return;
	}
	bzero(dp, sizeof(*dp));
#if defined(__FreeBSD__) && __FreeBSD_version >= 500000
	callout_init(&dp->dad_timer_ch, 0);
#elif defined(__NetBSD__) || (defined(__FreeBSD__) && __FreeBSD__ >= 3)
	callout_init(&dp->dad_timer_ch);
#elif defined(__OpenBSD__)
	bzero(&dp->dad_timer_ch, sizeof(dp->dad_timer_ch));
#endif
	TAILQ_INSERT_TAIL(&dadq, (struct dadq *)dp, dad_list);

	nd6log((LOG_DEBUG, "%s: starting DAD for %s\n", if_name(ifa->ifa_ifp),
	    ip6_sprintf(&ia->ia_addr.sin6_addr)));

	/*
	 * Send NS packet for DAD, ip6_dad_count times.
	 * Note that we must delay the first transmission, if this is the
	 * first packet to be sent from the interface after interface
	 * (re)initialization.
	 */
	dp->dad_ifa = ifa;
	IFAREF(ifa);	/* just for safety */
	dp->dad_count = ip6_dad_count;
	dp->dad_ns_icount = dp->dad_na_icount = 0;
	dp->dad_ns_ocount = dp->dad_ns_tcount = 0;
	if (tick == NULL) {
		nd6_dad_ns_output(dp, ifa);
		nd6_dad_starttimer(dp,
		    (long)ND_IFINFO(ifa->ifa_ifp)->retrans * hz / 1000);
	} else {
		int ntick;

#ifdef __bsdi__
#define arc4random	random
#endif
		if (*tick == 0)
			ntick = arc4random() % (MAX_RTR_SOLICITATION_DELAY * hz);
		else
			ntick = *tick + arc4random() % (hz / 2);
#ifdef __bsdi__
#undef arc4random
#endif
		*tick = ntick;
		nd6_dad_starttimer(dp, ntick);
	}
}

/*
 * terminate DAD unconditionally.  used for address removals.
 */
void
nd6_dad_stop(ifa)
	struct ifaddr *ifa;
{
	struct dadq *dp;

	if (!dad_init)
		return;
	dp = nd6_dad_find(ifa);
	if (!dp) {
		/* DAD wasn't started yet */
		return;
	}

	nd6_dad_stoptimer(dp);

	TAILQ_REMOVE(&dadq, (struct dadq *)dp, dad_list);
	free(dp, M_IP6NDP);
	dp = NULL;
	IFAFREE(ifa);
}

static void
nd6_dad_timer(ifa)
	struct ifaddr *ifa;
{
	int s;
	struct in6_ifaddr *ia = (struct in6_ifaddr *)ifa;
	struct dadq *dp;

#if defined(__NetBSD__) || defined(__OpenBSD__)
	s = splsoftnet();	/* XXX */
#else
	s = splnet();		/* XXX */
#endif

	/* Sanity check */
	if (ia == NULL) {
		log(LOG_ERR, "nd6_dad_timer: called with null parameter\n");
		goto done;
	}
	dp = nd6_dad_find(ifa);
	if (dp == NULL) {
		log(LOG_ERR, "nd6_dad_timer: DAD structure not found\n");
		goto done;
	}
	if (ia->ia6_flags & IN6_IFF_DUPLICATED) {
		log(LOG_ERR, "nd6_dad_timer: called with duplicated address "
			"%s(%s)\n",
			ip6_sprintf(&ia->ia_addr.sin6_addr),
			ifa->ifa_ifp ? if_name(ifa->ifa_ifp) : "???");
		goto done;
	}
	if ((ia->ia6_flags & IN6_IFF_TENTATIVE) == 0) {
		log(LOG_ERR, "nd6_dad_timer: called with non-tentative address "
			"%s(%s)\n",
			ip6_sprintf(&ia->ia_addr.sin6_addr),
			ifa->ifa_ifp ? if_name(ifa->ifa_ifp) : "???");
		goto done;
	}

	/* timeouted with IFF_{RUNNING,UP} check */
	if (dp->dad_ns_tcount > dad_maxtry) {
		nd6log((LOG_INFO, "%s: could not run DAD, driver problem?\n",
		    if_name(ifa->ifa_ifp)));

		TAILQ_REMOVE(&dadq, (struct dadq *)dp, dad_list);
		free(dp, M_IP6NDP);
		dp = NULL;
#if defined(MIP6) && defined(MIP6_HOME_AGENT)
		if (mip6_dad_success(ifa) == ENOENT)
#endif /* MIP6 && MIP6_HOME_AGENT */
		IFAFREE(ifa);
		goto done;
	}

	/* Need more checks? */
	if (dp->dad_ns_ocount < dp->dad_count) {
		/*
		 * We have more NS to go.  Send NS packet for DAD.
		 */
		nd6_dad_ns_output(dp, ifa);
		nd6_dad_starttimer(dp,
		    (long)ND_IFINFO(ifa->ifa_ifp)->retrans * hz / 1000);
	} else {
		/*
		 * We have transmitted sufficient number of DAD packets.
		 * See what we've got.
		 */
		int duplicate;

		duplicate = 0;

		if (dp->dad_na_icount) {
			/*
			 * the check is in nd6_dad_na_input(),
			 * but just in case
			 */
			duplicate++;
		}

		if (dp->dad_ns_icount) {
#if 0 /* heuristics */
			/*
			 * if
			 * - we have sent many(?) DAD NS, and
			 * - the number of NS we sent equals to the
			 *   number of NS we've got, and
			 * - we've got no NA
			 * we may have a faulty network card/driver which
			 * loops back multicasts to myself.
			 */
			if (3 < dp->dad_count
			 && dp->dad_ns_icount == dp->dad_count
			 && dp->dad_na_icount == 0) {
				log(LOG_INFO, "DAD questionable for %s(%s): "
				    "network card loops back multicast?\n",
				    ip6_sprintf(&ia->ia_addr.sin6_addr),
				    if_name(ifa->ifa_ifp));
				/* XXX consider it a duplicate or not? */
				/* duplicate++; */
			} else {
				/* We've seen NS, means DAD has failed. */
				duplicate++;
			}
#else
			/* We've seen NS, means DAD has failed. */
			duplicate++;
#endif
		}

		if (duplicate) {
			/* (*dp) will be freed in nd6_dad_duplicated() */
			dp = NULL;
			nd6_dad_duplicated(ifa);
		} else {
			/*
			 * We are done with DAD.  No NA came, no NS came.
			 * duplicated address found.
			 */
			ia->ia6_flags &= ~IN6_IFF_TENTATIVE;

			nd6log((LOG_DEBUG,
			    "%s: DAD complete for %s - no duplicates found\n",
			    if_name(ifa->ifa_ifp),
			    ip6_sprintf(&ia->ia_addr.sin6_addr)));

			TAILQ_REMOVE(&dadq, (struct dadq *)dp, dad_list);
			free(dp, M_IP6NDP);
			dp = NULL;
#if defined(MIP6) && defined(MIP6_HOME_AGENT)
			if (mip6_dad_success(ifa) == ENOENT)
#endif /* MIP6 && MIP6_HOME_AGENT */
			IFAFREE(ifa);
		}
	}

done:
	splx(s);
}

void
nd6_dad_duplicated(ifa)
	struct ifaddr *ifa;
{
	struct in6_ifaddr *ia = (struct in6_ifaddr *)ifa;
	struct dadq *dp;

	dp = nd6_dad_find(ifa);
	if (dp == NULL) {
		log(LOG_ERR, "nd6_dad_duplicated: DAD structure not found\n");
		return;
	}

	log(LOG_ERR, "%s: DAD detected duplicate IPv6 address %s: "
	    "NS in/out=%d/%d, NA in=%d\n",
	    if_name(ifa->ifa_ifp), ip6_sprintf(&ia->ia_addr.sin6_addr),
	    dp->dad_ns_icount, dp->dad_ns_ocount, dp->dad_na_icount);

	ia->ia6_flags &= ~IN6_IFF_TENTATIVE;
	ia->ia6_flags |= IN6_IFF_DUPLICATED;

	/* We are done with DAD, with duplicated address found. (failure) */
	nd6_dad_stoptimer(dp);

	log(LOG_ERR, "%s: DAD complete for %s - duplicate found\n",
	    if_name(ifa->ifa_ifp), ip6_sprintf(&ia->ia_addr.sin6_addr));
	log(LOG_ERR, "%s: manual intervention required\n",
	    if_name(ifa->ifa_ifp));

	TAILQ_REMOVE(&dadq, (struct dadq *)dp, dad_list);
	free(dp, M_IP6NDP);
	dp = NULL;
#if defined(MIP6) && defined(MIP6_HOME_AGENT)
	if (mip6_dad_duplicated(ifa) == ENOENT)
#endif /* MIP6 && MIP6_HOME_AGENT */
	IFAFREE(ifa);
}

static void
nd6_dad_ns_output(dp, ifa)
	struct dadq *dp;
	struct ifaddr *ifa;
{
	struct in6_ifaddr *ia = (struct in6_ifaddr *)ifa;
	struct ifnet *ifp = ifa->ifa_ifp;

	dp->dad_ns_tcount++;
	if ((ifp->if_flags & IFF_UP) == 0) {
#if 0
		printf("%s: interface down?\n", if_name(ifp));
#endif
		return;
	}
	if ((ifp->if_flags & IFF_RUNNING) == 0) {
#if 0
		printf("%s: interface not running?\n", if_name(ifp));
#endif
		return;
	}

	dp->dad_ns_ocount++;
	nd6_ns_output(ifp, NULL, &ia->ia_addr, NULL, 1);
}

static void
nd6_dad_ns_input(ifa)
	struct ifaddr *ifa;
{
	struct in6_ifaddr *ia;
	struct ifnet *ifp;
	const struct in6_addr *taddr6;
	struct dadq *dp;
	int duplicate;

	if (!ifa)
		panic("ifa == NULL in nd6_dad_ns_input");

	ia = (struct in6_ifaddr *)ifa;
	ifp = ifa->ifa_ifp;
	taddr6 = &ia->ia_addr.sin6_addr;
	duplicate = 0;
	dp = nd6_dad_find(ifa);

	/* Quickhack - completely ignore DAD NS packets */
	if (dad_ignore_ns) {
		nd6log((LOG_INFO,
		    "nd6_dad_ns_input: ignoring DAD NS packet for "
		    "address %s(%s)\n", ip6_sprintf(taddr6),
		    if_name(ifa->ifa_ifp)));
		return;
	}

	/*
	 * if I'm yet to start DAD, someone else started using this address
	 * first.  I have a duplicate and you win.
	 */
	if (!dp || dp->dad_ns_ocount == 0)
		duplicate++;

	/* XXX more checks for loopback situation - see nd6_dad_timer too */

	if (duplicate) {
		dp = NULL;	/* will be freed in nd6_dad_duplicated() */
		nd6_dad_duplicated(ifa);
	} else {
		/*
		 * not sure if I got a duplicate.
		 * increment ns count and see what happens.
		 */
		if (dp)
			dp->dad_ns_icount++;
	}
}

static void
nd6_dad_na_input(ifa)
	struct ifaddr *ifa;
{
	struct dadq *dp;

	if (!ifa)
		panic("ifa == NULL in nd6_dad_na_input");

	dp = nd6_dad_find(ifa);
	if (dp)
		dp->dad_na_icount++;

	/* remove the address. */
	nd6_dad_duplicated(ifa);
}
