# $NetBSD: Makefile,v 1.1.12.1 2000/09/26 15:50:40 simonb Exp $

PROG = bootxx_lfs
CPPFLAGS += -DLIBSA_SINGLE_FILESYSTEM=lfs \
	    -DBOOTXX_FS_NAME='"LFS"' \
	    -DBOOTXX_RAID1_SUPPORT \
	    -DNO_GETCHAR

# not yet desired: -DBOOTXX_FS_TYPE=FS_BSDLFS

PRIMARY_MAX_LOAD!=	expr 8192 - 512

.include "../Makefile.bootxx"
