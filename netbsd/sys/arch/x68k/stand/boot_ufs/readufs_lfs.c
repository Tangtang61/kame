/*	from Id: readufs_lfs.c,v 1.5 2002/01/26 16:26:44 itohy Exp 	*/

/*
 * FS specific support for 4.4BSD Log-structured Filesystem
 *
 * Written by ITOH, Yasufumi (itohy@netbsd.org).
 * Public domain.
 *
 * Intended to be used for boot programs (first stage).
 * DON'T ADD ANY FANCY FEATURE.  THIS SHALL BE COMPACT.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <ufs/ufs/dinode.h>
#include <ufs/lfs/lfs.h>

#include "readufs.h"

static int get_lfs_inode __P((ino_t ino, struct dinode *dibuf));

static struct dinode	ifile_dinode;

#define fsi	(*ufsinfo)
#define fsi_lfs	fsi.fs_u.u_lfs

/*
 * Read and check superblock.
 * If it is an LFS, save information from the superblock.
 */
int
try_lfs()
{
	struct ufs_info	*ufsinfo = &ufs_info;
	struct dlfs	sblk, sblk2;
	struct dlfs	*s = &sblk;
	ufs_daddr_t	sbpos;
	int		fsbshift;

#ifdef DEBUG_WITH_STDIO
	printf("trying LFS\n");
#endif
	sbpos =  btodb(LFS_LABELPAD);

	/* read primary superblock */
	for (;;) {
#ifdef DEBUG_WITH_STDIO
		printf("LFS: reading primary sblk at: 0x%x\n", sbpos);
#endif
		RAW_READ(&sblk, sbpos, sizeof sblk);

#ifdef DEBUG_WITH_STDIO
		printf("LFS: sblk: magic: 0x%x, version: %d\n",
			sblk.dlfs_magic, sblk.dlfs_version);
#endif

		if (sblk.dlfs_magic != LFS_MAGIC)
			return 1;

#ifdef DEBUG_WITH_STDIO
		printf("LFS: bsize %d, fsize %d, bshift %d, blktodb %d, fsbtodb %d, inopf %d, inopb %d\n",
		    sblk.dlfs_bsize, sblk.dlfs_fsize,
		    sblk.dlfs_bshift, sblk.dlfs_blktodb, sblk.dlfs_fsbtodb,
		    sblk.dlfs_inopf, sblk.dlfs_inopb);
#endif
		if ((fsi_lfs.version = sblk.dlfs_version) == 1) {
			fsbshift = 0;
			break;
		} else {
			ufs_daddr_t	sbpos1;
#if 0
			fsbshift = sblk.dlfs_bshift - sblk.dlfs_blktodb + sblk.dlfs_fsbtodb - DEV_BSHIFT;
#endif
			fsbshift = sblk.dlfs_fsbtodb;
			sbpos1 = sblk.dlfs_sboffs[0] << fsbshift;
			if (sbpos == sbpos1)
				break;
#ifdef DEBUG_WITH_STDIO
			printf("LFS: correcting primary sblk location\n");
#endif
			sbpos = sbpos1;
		}
	}

#ifdef DEBUG_WITH_STDIO
	printf("fsbshift: %d\n", fsbshift);
	printf("sboff[1]: %d\n", sblk.dlfs_sboffs[1]);
#endif

	if (sblk.dlfs_sboffs[1] > 0) {
#ifdef DEBUG_WITH_STDIO
		printf("LFS: reading secondary sblk at: 0x%x\n",
		    sblk.dlfs_sboffs[1] << fsbshift);
#endif
		/* read secondary superblock */
		RAW_READ(&sblk2, sblk.dlfs_sboffs[1] << fsbshift, sizeof sblk2);

#ifdef DEBUG_WITH_STDIO
		printf("LFS: sblk2: magic: 0x%x, version: %d\n",
			sblk2.dlfs_magic, sblk2.dlfs_version);
#endif

		if (sblk2.dlfs_magic == LFS_MAGIC) {
			if (fsi_lfs.version == 1) {
				if (sblk.dlfs_otstamp > sblk2.dlfs_otstamp)
					s = &sblk2;
			} else {
				if (sblk.dlfs_serial > sblk2.dlfs_serial)
					s = &sblk2;
			}
		}
	}

	/* This partition looks like an LFS. */
	fsi.get_inode = get_lfs_inode;
	/*
	 * version 1: disk addr is in disk sector --- no shifting
	 * version 2: disk addr is in fragment
	 */
	fsi.fsbtodb = fsbshift;

	/* Get information from the superblock. */
	fsi.bsize = s->dlfs_bsize;
	fsi.nindir = s->dlfs_nindir;
	fsi_lfs.idaddr = s->dlfs_idaddr;
#if 0
	fsi_lfs.ibsize = (fsi_lfs.version == 1) ? s->dlfs_bsize : s->dlfs_fsize;
#else	/* simplify calculation to reduce code size */
	/* use fsi.bsize (larger then needed for v2, but probably no harm) */
#endif

	/*
	 * version 1: number of inode per block
	 * version 2: number of inode per fragment (but in dlfs_inopb)
	 */
	fsi_lfs.inopb = s->dlfs_inopb;

	fsi_lfs.ifpb = s->dlfs_ifpb;
	fsi_lfs.ioffset = s->dlfs_cleansz + s->dlfs_segtabsz;

	/* ifile is always used to look-up other inodes, so keep its inode. */
	if (get_lfs_inode(LFS_IFILE_INUM, &ifile_dinode))
		return 1;	/* OOPS, failed to find inode of ifile! */

	fsi.fstype = UFSTYPE_LFS;

	return 0;
}

/*
 * Get inode from disk.
 */
static int
get_lfs_inode(ino, dibuf)
	ino_t ino;
	struct dinode *dibuf;
{
	struct ufs_info *ufsinfo = &ufs_info;
	ufs_daddr_t daddr;
	char *buf = alloca(fsi.bsize);
	struct dinode *di, *diend;
	int i;

	/* Get fs block which contains the specified inode. */
	if (ino == LFS_IFILE_INUM)
		daddr = fsi_lfs.idaddr;
	else {
#ifdef DEBUG_WITH_STDIO
		printf("LFS: ino: %d\nifpb: %d, bsize: %d\n",
			ino, fsi_lfs.ifpb, fsi.bsize);
#endif
		ufs_read(&ifile_dinode, buf, 
			 ino / fsi_lfs.ifpb + fsi_lfs.ioffset,
			 fsi.bsize);
		i = ino % fsi_lfs.ifpb;
		daddr = (fsi_lfs.version == 1) ?
		    ((IFILE_V1 *) buf + i)->if_daddr
		    : ((IFILE *) buf + i)->if_daddr;
	}
#ifdef DEBUG_WITH_STDIO
	printf("LFS(%d): daddr: %d\n", ino, daddr);
#endif

	if (daddr == LFS_UNUSED_DADDR)
		return 1;

	/* Read the inode block. */
	RAW_READ(buf, daddr << fsi.fsbtodb,
#if 0
	fsi_lfs.ibsize
#else	/* simplify calculation to reduce code size */
	fsi.bsize
#endif
	);

	/* Search for the inode. */
	di = (struct dinode *) buf;
	diend = di + fsi_lfs.inopb;

	for ( ; di < diend; di++)
		if (di->di_inumber == ino)
			goto found;
	/* not found */
	return 1;

found:
#ifdef DEBUG_WITH_STDIO
	printf("LFS: dinode(%d): mode 0%o, nlink %d, inumber %d, size %d, uid %d, gid %d, db[0] %d\n",
		ino, di->di_mode, di->di_nlink, di->di_inumber,
		(int)di->di_size, di->di_uid, di->di_gid, di->di_db[0]);
#endif

	*dibuf = *di;

	return 0;
}