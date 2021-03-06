/*-
 * Copyright (c) 2004 Pawel Jakub Dawidek <pjd@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/geom/raid3/g_raid3.h,v 1.1.2.2 2004/10/15 06:11:28 pjd Exp $
 */

#ifndef	_G_RAID3_H_
#define	_G_RAID3_H_

#include <sys/endian.h>
#include <sys/md5.h>

#define	G_RAID3_CLASS_NAME	"RAID3"

#define	G_RAID3_MAGIC		"GEOM::RAID3"
/*
 * Version history:
 * 0 - Initial version number.
 * 1 - Added 'round-robin reading' algorithm.
 * 2 - Added 'verify reading' algorithm.
 */
#define	G_RAID3_VERSION		2

#define	G_RAID3_DISK_FLAG_DIRTY		0x0000000000000001ULL
#define	G_RAID3_DISK_FLAG_SYNCHRONIZING	0x0000000000000002ULL
#define	G_RAID3_DISK_FLAG_FORCE_SYNC	0x0000000000000004ULL
#define	G_RAID3_DISK_FLAG_HARDCODED	0x0000000000000008ULL
#define	G_RAID3_DISK_FLAG_MASK		(G_RAID3_DISK_FLAG_DIRTY |	\
					 G_RAID3_DISK_FLAG_SYNCHRONIZING | \
					 G_RAID3_DISK_FLAG_FORCE_SYNC)

#define	G_RAID3_DEVICE_FLAG_NOAUTOSYNC	0x0000000000000001ULL
#define	G_RAID3_DEVICE_FLAG_ROUND_ROBIN	0x0000000000000002ULL
#define	G_RAID3_DEVICE_FLAG_VERIFY	0x0000000000000004ULL
#define	G_RAID3_DEVICE_FLAG_MASK	(G_RAID3_DEVICE_FLAG_NOAUTOSYNC | \
					 G_RAID3_DEVICE_FLAG_ROUND_ROBIN | \
					 G_RAID3_DEVICE_FLAG_VERIFY)

#ifdef _KERNEL
extern u_int g_raid3_debug;

#define	G_RAID3_DEBUG(lvl, ...)	do {					\
	if (g_raid3_debug >= (lvl)) {					\
		printf("GEOM_RAID3");					\
		if (g_raid3_debug > 0)					\
			printf("[%u]", lvl);				\
		printf(": ");						\
		printf(__VA_ARGS__);					\
		printf("\n");						\
	}								\
} while (0)
#define	G_RAID3_LOGREQ(lvl, bp, ...)	do {				\
	if (g_raid3_debug >= (lvl)) {					\
		printf("GEOM_RAID3");					\
		if (g_raid3_debug > 0)					\
			printf("[%u]", lvl);				\
		printf(": ");						\
		printf(__VA_ARGS__);					\
		printf(" ");						\
		g_print_bio(bp);					\
		printf("\n");						\
	}								\
} while (0)

#define	G_RAID3_BIO_CFLAG_REGULAR	0x01
#define	G_RAID3_BIO_CFLAG_SYNC		0x02
#define	G_RAID3_BIO_CFLAG_PARITY	0x04
#define	G_RAID3_BIO_CFLAG_NODISK	0x08
#define	G_RAID3_BIO_CFLAG_REGSYNC	0x10
#define	G_RAID3_BIO_CFLAG_MASK		(G_RAID3_BIO_CFLAG_REGULAR |	\
					 G_RAID3_BIO_CFLAG_SYNC |	\
					 G_RAID3_BIO_CFLAG_PARITY |	\
					 G_RAID3_BIO_CFLAG_NODISK |	\
					 G_RAID3_BIO_CFLAG_REGSYNC)

#define	G_RAID3_BIO_PFLAG_DEGRADED	0x01
#define	G_RAID3_BIO_PFLAG_NOPARITY	0x02
#define	G_RAID3_BIO_PFLAG_VERIFY	0x04
#define	G_RAID3_BIO_PFLAG_MASK		(G_RAID3_BIO_PFLAG_DEGRADED |	\
					 G_RAID3_BIO_PFLAG_NOPARITY |	\
					 G_RAID3_BIO_PFLAG_VERIFY)

/*
 * Informations needed for synchronization.
 */
struct g_raid3_disk_sync {
	struct g_consumer *ds_consumer;	/* Consumer connected to our device. */
	off_t		 ds_offset;	/* Offset of next request to send. */
	off_t		 ds_offset_done; /* Offset of already synchronized
					   region. */
	off_t		 ds_resync;	/* Resynchronize from this offset. */
	u_int		 ds_syncid;	/* Disk's synchronization ID. */
	u_char		*ds_data;
};

/*
 * Informations needed for synchronization.
 */
struct g_raid3_device_sync {
	struct g_geom	*ds_geom;	/* Synchronization geom. */
};

#define	G_RAID3_DISK_STATE_NODISK		0
#define	G_RAID3_DISK_STATE_NONE			1
#define	G_RAID3_DISK_STATE_NEW			2
#define	G_RAID3_DISK_STATE_ACTIVE		3
#define	G_RAID3_DISK_STATE_STALE		4
#define	G_RAID3_DISK_STATE_SYNCHRONIZING	5
#define	G_RAID3_DISK_STATE_DISCONNECTED		6
#define	G_RAID3_DISK_STATE_DESTROY		7
struct g_raid3_disk {
	u_int		 d_no;		/* Disk number. */
	struct g_consumer *d_consumer;	/* Consumer. */
	struct g_raid3_softc *d_softc;	/* Back-pointer to softc. */
	int		 d_state;	/* Disk state. */
	uint64_t	 d_flags;	/* Additional flags. */
	struct g_raid3_disk_sync d_sync; /* Sync information. */
	LIST_ENTRY(g_raid3_disk) d_next;
};
#define	d_name	d_consumer->provider->name

#define	G_RAID3_EVENT_DONTWAIT	0x1
#define	G_RAID3_EVENT_WAIT	0x2
#define	G_RAID3_EVENT_DEVICE	0x4
#define	G_RAID3_EVENT_DONE	0x8
struct g_raid3_event {
	struct g_raid3_disk	*e_disk;
	int			 e_state;
	int			 e_flags;
	int			 e_error;
	TAILQ_ENTRY(g_raid3_event) e_next;
};

#define	G_RAID3_DEVICE_FLAG_DESTROY	0x0100000000000000ULL
#define	G_RAID3_DEVICE_FLAG_WAIT	0x0200000000000000ULL

#define	G_RAID3_DEVICE_STATE_STARTING		0
#define	G_RAID3_DEVICE_STATE_DEGRADED		1
#define	G_RAID3_DEVICE_STATE_COMPLETE		2

#define	G_RAID3_BUMP_ON_FIRST_WRITE		1
#define	G_RAID3_BUMP_IMMEDIATELY		2

struct g_raid3_softc {
	u_int		sc_state;	/* Device state. */
	uint64_t	sc_mediasize;	/* Device size. */
	uint32_t	sc_sectorsize;	/* Sector size. */
	uint64_t	sc_flags;	/* Additional flags. */

	struct g_geom	*sc_geom;
	struct g_provider *sc_provider;

	uint32_t	sc_id;		/* Device unique ID. */

	struct bio_queue_head sc_queue;
	struct mtx	 sc_queue_mtx;
	struct proc	*sc_worker;

	struct g_raid3_disk *sc_disks;
	u_int		sc_ndisks;	/* Number of disks. */
	u_int		sc_round_robin;
	struct g_raid3_disk *sc_syncdisk;

	uma_zone_t	sc_zone_64k;
	uma_zone_t	sc_zone_16k;
	uma_zone_t	sc_zone_4k;

	u_int		sc_syncid;	/* Synchronization ID. */
	int		sc_bump_syncid;
	struct g_raid3_device_sync sc_sync;

	TAILQ_HEAD(, g_raid3_event) sc_events;
	struct mtx	sc_events_mtx;

	struct callout	sc_callout;
};
#define	sc_name	sc_geom->name

const char *g_raid3_get_diskname(struct g_raid3_disk *disk);
u_int g_raid3_ndisks(struct g_raid3_softc *sc, int state);
int g_raid3_destroy(struct g_raid3_softc *sc, boolean_t force);
int g_raid3_event_send(void *arg, int state, int flags);
struct g_raid3_metadata;
void g_raid3_fill_metadata(struct g_raid3_disk *disk,
    struct g_raid3_metadata *md);
int g_raid3_clear_metadata(struct g_raid3_disk *disk);
void g_raid3_update_metadata(struct g_raid3_disk *disk);

g_ctl_req_t g_raid3_config;
#endif	/* _KERNEL */

struct g_raid3_metadata {
	char		md_magic[16];	/* Magic value. */
	uint32_t	md_version;	/* Version number. */
	char		md_name[16];	/* Device name. */
	uint32_t	md_id;		/* Device unique ID. */
	uint16_t	md_no;		/* Component number. */
	uint16_t	md_all;		/* Number of disks in device. */
	uint32_t	md_syncid;	/* Synchronization ID. */
	uint64_t	md_mediasize;	/* Size of whole device. */
	uint32_t	md_sectorsize;	/* Sector size. */
	uint64_t	md_sync_offset;	/* Synchronized offset. */
	uint64_t	md_mflags;	/* Additional device flags. */
	uint64_t	md_dflags;	/* Additional disk flags. */
	char		md_provider[16]; /* Hardcoded provider. */
	u_char		md_hash[16];	/* MD5 hash. */
};
static __inline void
raid3_metadata_encode(struct g_raid3_metadata *md, u_char *data)
{
	MD5_CTX ctx;

	bcopy(md->md_magic, data, 16);
	le32enc(data + 16, md->md_version);
	bcopy(md->md_name, data + 20, 16);
	le32enc(data + 36, md->md_id);
	le16enc(data + 40, md->md_no);
	le16enc(data + 42, md->md_all);
	le32enc(data + 44, md->md_syncid);
	le64enc(data + 48, md->md_mediasize);
	le32enc(data + 56, md->md_sectorsize);
	le64enc(data + 60, md->md_sync_offset);
	le64enc(data + 68, md->md_mflags);
	le64enc(data + 76, md->md_dflags);
	bcopy(md->md_provider, data + 84, 16);
	MD5Init(&ctx);
	MD5Update(&ctx, data, 100);
	MD5Final(md->md_hash, &ctx);
	bcopy(md->md_hash, data + 100, 16);
}
static __inline int
raid3_metadata_decode(const u_char *data, struct g_raid3_metadata *md)
{
	MD5_CTX ctx;

	bcopy(data, md->md_magic, 16);
	md->md_version = le32dec(data + 16);
	bcopy(data + 20, md->md_name, 16);
	md->md_id = le32dec(data + 36);
	md->md_no = le16dec(data + 40);
	md->md_all = le16dec(data + 42);
	md->md_syncid = le32dec(data + 44);
	md->md_mediasize = le64dec(data + 48);
	md->md_sectorsize = le32dec(data + 56);
	md->md_sync_offset = le64dec(data + 60);
	md->md_mflags = le64dec(data + 68);
	md->md_dflags = le64dec(data + 76);
	bcopy(data + 84, md->md_provider, 16);
	bcopy(data + 100, md->md_hash, 16);
	MD5Init(&ctx);
	MD5Update(&ctx, data, 100);
	MD5Final(md->md_hash, &ctx);
	if (bcmp(md->md_hash, data + 100, 16) != 0)
		return (EINVAL);
	return (0);
}

static __inline void
raid3_metadata_dump(const struct g_raid3_metadata *md)
{
	static const char hex[] = "0123456789abcdef";
	char hash[16 * 2 + 1];
	u_int i;

	printf("     magic: %s\n", md->md_magic);
	printf("   version: %u\n", (u_int)md->md_version);
	printf("      name: %s\n", md->md_name);
	printf("        id: %u\n", (u_int)md->md_id);
	printf("        no: %u\n", (u_int)md->md_no);
	printf("       all: %u\n", (u_int)md->md_all);
	printf("    syncid: %u\n", (u_int)md->md_syncid);
	printf(" mediasize: %jd\n", (intmax_t)md->md_mediasize);
	printf("sectorsize: %u\n", (u_int)md->md_sectorsize);
	printf("syncoffset: %jd\n", (intmax_t)md->md_sync_offset);
	printf("    mflags:");
	if (md->md_mflags == 0)
		printf(" NONE");
	else {
		if ((md->md_mflags & G_RAID3_DEVICE_FLAG_NOAUTOSYNC) != 0)
			printf(" NOAUTOSYNC");
		if ((md->md_mflags & G_RAID3_DEVICE_FLAG_ROUND_ROBIN) != 0)
			printf(" ROUND-ROBIN");
		if ((md->md_mflags & G_RAID3_DEVICE_FLAG_VERIFY) != 0)
			printf(" VERIFY");
	}
	printf("\n");
	printf("    dflags:");
	if (md->md_dflags == 0)
		printf(" NONE");
	else {
		if ((md->md_dflags & G_RAID3_DISK_FLAG_DIRTY) != 0)
			printf(" DIRTY");
		if ((md->md_dflags & G_RAID3_DISK_FLAG_SYNCHRONIZING) != 0)
			printf(" SYNCHRONIZING");
		if ((md->md_dflags & G_RAID3_DISK_FLAG_FORCE_SYNC) != 0)
			printf(" FORCE_SYNC");
	}
	printf("\n");
	printf("hcprovider: %s\n", md->md_provider);
	bzero(hash, sizeof(hash));
	for (i = 0; i < 16; i++) {
		hash[i * 2] = hex[md->md_hash[i] >> 4];
		hash[i * 2 + 1] = hex[md->md_hash[i] & 0x0f];
	}
	printf("  MD5 hash: %s\n", hash);
}
#endif	/* !_G_RAID3_H_ */
