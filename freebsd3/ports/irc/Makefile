# New ports collection makefile for:	Irc Server
# Date created:		23 August 1994
# Whom:			torstenb
#
# $FreeBSD: ports/irc/irc/Makefile,v 1.23 2000/01/26 21:08:15 sumikawa Exp $
# $KAME: Makefile,v 1.4 2000/06/25 10:58:34 sumikawa Exp $
#

PORTNAME=	irc
PORTVERSION=	2.10.3
#CATEGORIES=	irc ipv6
CATEGORIES=	irc
MASTER_SITES=	ftp://ftp.funet.fi/pub/networking/services/irc/server/ \
		ftp://ftp.irc.org/irc/server/ \
		ftp://tau.ac.il/pub/unix/irc/server/ \
		ftp://ftp.ut.ee/pub/unix/irc/server/ \
		ftp://ftp.ntua.gr/pub/net/irc/server/
DISTNAME=	${PORTNAME}${PORTVERSION}

MAINTAINER=	sumikawa@kame.net

# the NOTE command is disabled by default
# ENABLE_NOTE=yes

# Syslog-Facility to use for logging
# default:
SYSLOG_FACILITY=LOG_LOCAL5

EXTRACT_SUFX=	.tgz
SCRIPTS_ENV=	SYSLOG_FACILITY="${SYSLOG_FACILITY}"

MAN1=		irc.1
MAN5=		iauth.conf.5
MAN8=		ircd.8 iauth.8 ircdwatch.8

GNU_CONFIGURE=	YES
CONFIGURE_ARGS=	--sysconfdir=${PREFIX}/etc/ircd
STARTUP_SCRIPT=	${PREFIX}/etc/rc.d/ircd.sh

.include <bsd.port.pre.mk>

#.if ${OSVERSION} >= 400014
CONFIGURE_ARGS+= --enable-ip6
#.endif

do-build:
	@(cd ${WRKSRC}/${MACHINE_ARCH}-unknown-freebsd${OSREL}; ${SETENV} ${MAKE_ENV} ${MAKE} ${MAKE_FLAGS} ${MAKEFILE} ${MAKE_ARGS} ${ALL_TARGET})

do-install:
	@(cd ${WRKSRC}/${MACHINE_ARCH}-unknown-freebsd${OSREL} && ${SETENV} ${MAKE_ENV} ${MAKE} ${MAKE_FLAGS} ${MAKEFILE} ${MAKE_ARGS} ${INSTALL_TARGET})

post-install:
	@if [ ! -f ${STARTUP_SCRIPT} ]; then				\
		echo "Installing ${STARTUP_SCRIPT} startup file.";	\
		echo '#!/bin/sh' > ${STARTUP_SCRIPT};			\
		echo "[ -f ${PREFIX}/etc/ircd/ircd.conf ] && ${PREFIX}/sbin/ircd && echo -n ' ircd'" >> ${STARTUP_SCRIPT} ;	\
		chmod 755 ${STARTUP_SCRIPT} ;				\
		chown bin.bin ${STARTUP_SCRIPT};			\
	fi

.include <bsd.port.post.mk>
