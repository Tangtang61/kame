# $FreeBSD: src/sys/modules/netgraph/cisco/Makefile,v 1.2 1999/11/28 18:53:05 bde Exp $
# $Whistle: Makefile,v 1.2 1999/01/19 19:39:20 archie Exp $

KMOD=		ng_cisco
SRCS= 		ng_cisco.c opt_inet.h opt_atalk.h opt_ipx.h
MAN8=		ng_cisco.8
KMODDEPS=	netgraph

IFACE_INET?=		1	# 0/1 - requires INET configured in kernel
IFACE_NETATALK?=	0	# 0/1 - requires NETATALK configured in kernel
IFACE_IPX?=		0	# 0/1 - requires IPX configured in kernel

CFLAGS+=	${PROTOS}

opt_inet.h:
	touch opt_inet.h
.if ${IFACE_INET} > 0
	echo "#define INET 1" > opt_inet.h
.endif

opt_atalk.h:
	touch opt_atalk.h
.if ${IFACE_NETATALK} > 0
	echo "#define NETATALK ${IFACE_NETATALK}" > opt_atalk.h
.endif

opt_ipx.h:
	touch opt_ipx.h
.if ${IFACE_IPX} > 0
	echo "#define IPX ${IFACE_IPX}" > opt_ipx.h
.endif

.include <bsd.kmod.mk>
