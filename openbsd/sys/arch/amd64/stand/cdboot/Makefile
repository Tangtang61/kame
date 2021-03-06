#	$OpenBSD: Makefile,v 1.2 2004/08/24 15:34:59 tom Exp $

.include "${.CURDIR}/../Makefile.inc"

MAN=	cdboot.8

.if ${MACHINE} == "amd64"
S	=${.CURDIR}/../../../..
SADIR=	${.CURDIR}/..

PROG=	cdboot
SRCS=	srt0.S cdboot.c conf.c
LD?=	ld
SIZE?=	size
LDFLAGS+=-melf_i386 -nostdlib -Bstatic -Ttext $(LINKADDR) -N -x -noinhibit-exec
LDFLAGS+=-L/usr/libdata 
INSTALL_STRIP=

.PATH:	${SADIR}/libsa
# i386 stuff (so, it will possibly load in the same 64k)
SRCS+=	machdep.c dev_i386.c exec_i386.c cmd_i386.c
SRCS+=	gidt.S alloca.S biosdev.c bioscons.c gateA20.c \
	memprobe.c diskprobe.c time.c biosprobe.c

.PATH:	${S}/stand/boot
SRCS+=	boot.c cmd.c vars.c bootarg.c

.PATH:	${S}/lib/libsa
.PATH:	${S}/lib/libkern	# for strl*
# stand routines
# not required: getfile.c gets.c globals.c strlcat.c
SRCS+=	alloc.c exit.c strcmp.c strlen.c \
	strncmp.c memcmp.c memcpy.c memset.c printf.c snprintf.c \
	strerror.c strncpy.c strtol.c ctime.c strlcpy.c
# io routines
# not required: ioctl.c write.c
SRCS+=	close.c closeall.c dev.c disklabel.c dkcksum.c fstat.c lseek.c \
	open.c read.c stat.c cread.c readdir.c cons.c loadfile.c
# boot filesystems
SRCS+=	ufs.c cd9660.c

.PATH:	${S}/lib/libz
SRCS+=	adler32.c crc32.c inflate.c inftrees.c

#LDADD=	${LIBSA} ${LIBZ}
#DPADD=	${LIBSA} ${LIBZ}

${PROG}: $(OBJS) $(DPADD)
	$(LD) $(LDFLAGS) -o ${PROG} $(OBJS) $(LDADD)
	@if [ -x ${.OBJDIR}/${PROG} ]; then \
		objcopy -O binary ${PROG} ${.OBJDIR}/.tmp;\
		mv -f ${.OBJDIR}/.tmp ${.OBJDIR}/${PROG}; \
		ls -l ${.OBJDIR}/${PROG}; \
	fi

.else
NOPROG=
.endif

.include <bsd.prog.mk>

CPPFLAGS+=-DBOOTMAGIC=$(BOOTMAGIC) ${DEBUGFLAGS}
CPPFLAGS+=-DSLOW -DSMALL -DNOBYFOUR -DNO_GZIP -DDYNAMIC_CRC_TABLE
CPPFLAGS+=-DLINKADDR=${LINKADDR} -I${S}/stand/boot
CFLAGS+=-m32 ${SACFLAGS} -D__INTERNAL_LIBSA_CREAD
CFLAGS+=-DOSREV=\"${OSREV}\" -DMACHINE=\"${MACHINE}\"
AFLAGS+=-m32
