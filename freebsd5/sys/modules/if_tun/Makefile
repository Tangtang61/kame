# $FreeBSD: src/sys/modules/if_tun/Makefile,v 1.26 2002/07/31 20:19:28 rwatson Exp $

.PATH: ${.CURDIR}/../../net

KMOD=	if_tun
SRCS=	if_tun.c opt_inet.h opt_mac.h vnode_if.h

opt_inet.h:
	echo "#define INET 1" > ${.TARGET}

.include <bsd.kmod.mk>
