# $Id: Makefile,v 1.7 2002/11/03 02:15:54 max Exp $
# $FreeBSD: src/sys/modules/netgraph/bluetooth/h4/Makefile,v 1.2 2002/11/21 07:21:44 imp Exp $

.PATH: ${.CURDIR}/../../../../netgraph/bluetooth/drivers/h4

CFLAGS+=	-g -I${.CURDIR}/../../../../netgraph/bluetooth/include \
		-I${.CURDIR}/../../../../netgraph/bluetooth/drivers/h4 \
		-DINVARIANTS=1 -DINVARIANT_SUPPORT=1
#		-DWITNESS=1 -DWITNESS_SKIPSPIN=1

KMOD=		ng_h4
SRCS=		ng_h4.c
MAN4=		ng_h4.4

.include <bsd.kmod.mk>

