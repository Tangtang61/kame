# $FreeBSD: src/sys/boot/pc98/boot0/Makefile,v 1.1.2.1 2000/09/11 09:19:46 kato Exp $

PROG=		boot0
OBJS=		boot0.o
NOMAN=
STRIP=
BINDIR?=	/boot
BINMODE=	444

ORG=		0x0000

boot0: boot0.out
	objcopy -S -O binary boot0.out ${.TARGET}

boot0.out: ${OBJS}
	${LD} -N -e start -Ttext ${ORG} -o ${.TARGET} ${OBJS}

CLEANFILES+= boot0.out

.include <bsd.prog.mk>
