# New ports collection makefile for:   mtr
# Date created:        12 August 1998
# Whom:                Bill Fumerola <billf@chc-chimes.com>
#
# $FreeBSD: ports/net/mtr/Makefile,v 1.17 2000/01/13 07:02:21 sumikawa Exp $
#

PORTNAME=	mtr
PORTVERSION=	0.42
CATEGORIES=	net
MASTER_SITES=	ftp://ftp.bitwizard.nl/mtr/ \
		http://www.giovannelli.it/~gmarco/files/

#.if defined(USE_INET6)
PATCHFILES=	mtr-042-v6-20000719.diff.gz
PATCH_SITES=	ftp://ftp.kame.net/pub/kame/misc/
PATCH_DIST_STRIP=	-p1
#.endif

MAINTAINER=	itojun@kame.net

USE_GMAKE=	yes
HAS_CONFIGURE=	yes
MAN8=	mtr.8

#.if defined(USE_INET6)
CONFIGURE_ARGS+=--enable-ipv6
#.else
#CONFIGURE_ARGS+=--disable-ipv6
#.endif

pre-patch:
.if !defined(USE_GTK)
	@${ECHO} ""
	@${ECHO} "If you want to compile with GTK support,"
	@${ECHO} "hit Ctrl-C right now and use \"make USE_GTK=yes\" "
	@${ECHO} ""
CONFIGURE_ARGS+=--with-gtk-prefix=/nopath
.else
USE_XLIB=	yes
LIB_DEPENDS=    gtk12.2:${PORTSDIR}/x11-toolkits/gtk12
CONFIGURE_ENV=	GTK_CONFIG="${X11BASE}/bin/gtk12-config"
.endif

do-install:
	${INSTALL} -c -s -m 4755 -o root -g wheel ${WRKSRC}/mtr ${PREFIX}/sbin
	${INSTALL_DATA} ${WRKSRC}/mtr.8 ${PREFIX}/man/man8

post-install:
	@${ECHO} ""
	@${ECHO} "${PREFIX}/sbin/mtr is setuid \"root\" "
	@${ECHO} ""
	@${ECHO} "Please read about potential security issues"
	@${ECHO} "in file ${WRKSRC}/SECURITY (not installed)"
	@${ECHO} ""

.include <bsd.port.mk>
