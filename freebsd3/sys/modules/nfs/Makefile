#	$Id: Makefile,v 1.11 1998/10/16 04:30:48 peter Exp $

.PATH: ${.CURDIR}/../../nfs
KMOD=	nfs
SRCS=	nfs_bio.c nfs_node.c nfs_nqlease.c nfs_serv.c nfs_socket.c \
	nfs_srvcache.c nfs_subs.c nfs_syscalls.c nfs_vfsops.c \
	nfs_vnops.c opt_inet.h opt_nfs.h opt_vmpage.h
NFS_INET?=	1	# 0/1 - requires INET to be configured in kernel
NOMAN=
VFS_KLD=
CFLAGS+= -DNFS
CLEANFILES= opt_inet.h opt_nfs.h opt_vmpage.h opt_posix.h

opt_inet.h:
	touch ${.TARGET}
.if ${NFS_INET} > 0
	echo "#define INET 1" > ${.TARGET}
.endif

opt_nfs.h opt_vmpage.h opt_posix.h:
	touch ${.TARGET}

.include <bsd.kmod.mk>
