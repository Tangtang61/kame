#	$NetBSD: Makefile,v 1.4 1999/02/13 02:54:43 lukem Exp $

PROG=	fdboot
SRCS=	${.CURDIR}/fdboot.s
MKMAN=	no
LIBCRT0=
INCL=	-I${.CURDIR} -I${.CURDIR}/..

CPPFLAGS+= ${INCL}

.include <bsd.prog.mk>
