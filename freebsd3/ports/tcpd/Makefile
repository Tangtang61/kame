# New ports collection makefile for:    tcpd
# Date created:         Sat Jun 24 01:18:09 JST 2000
# Whom:			sumikawa@kame.net
#
# $KAME: Makefile,v 1.2 2000/06/25 10:58:46 sumikawa Exp $
#

PORTNAME=	tcpd
PORTVERSION=	0.0.2
CATEGORIES=	net
MASTER_SITES=	ftp://ftp.pld.org.pl/software/tcpd/

MAINTAINER=	sumikawa@kame.net

GNU_CONFIGURE=	yes
CONFIGURE_ARGS=	--mandir=${LOCALBASE}/man

MAN3=		hosts_access.3 
MAN5=		hosts_access.5 hosts_options.5
MAN8=		tcpd.8 tcpd_check.8

.include <bsd.port.mk>
