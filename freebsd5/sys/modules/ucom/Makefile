# $FreeBSD: src/sys/modules/ucom/Makefile,v 1.3 2004/06/27 12:33:17 imp Exp $

S=	${.CURDIR}/../..
.PATH: $S/dev/usb

KMOD=	ucom
SRCS=	ucom.c ucomvar.h opt_usb.h device_if.h bus_if.h vnode_if.h usbdevs.h

.include <bsd.kmod.mk>
