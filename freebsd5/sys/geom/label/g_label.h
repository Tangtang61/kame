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
 * $FreeBSD: src/sys/geom/label/g_label.h,v 1.3 2004/07/13 12:01:29 pjd Exp $
 */

#ifndef	_G_LABEL_H_
#define	_G_LABEL_H_

#include <sys/endian.h>

#define	G_LABEL_CLASS_NAME	"LABEL"

#define	G_LABEL_MAGIC		"GEOM::LABEL"
#define	G_LABEL_VERSION		1
#define	G_LABEL_DIR		"label"

#ifdef _KERNEL
extern u_int g_label_debug;

#define	G_LABEL_DEBUG(lvl, ...)	do {					\
	if (g_label_debug >= (lvl)) {					\
		printf("GEOM_LABEL");					\
		if (g_label_debug > 0)					\
			printf("[%u]", lvl);				\
		printf(": ");						\
		printf(__VA_ARGS__);					\
		printf("\n");						\
	}								\
} while (0)

typedef void g_label_taste_t (struct g_consumer *cp, char *label, size_t size);

struct g_label_desc {
	g_label_taste_t	*ld_taste;
	char		*ld_dir;
};

/* Supported labels. */
extern const struct g_label_desc g_label_ufs;
extern const struct g_label_desc g_label_iso9660;
extern const struct g_label_desc g_label_msdosfs;
#endif	/* _KERNEL */

struct g_label_metadata {
	char		md_magic[16];	/* Magic value. */
	uint32_t	md_version;	/* Version number. */
	char		md_label[16];	/* Label. */
};
static __inline void
label_metadata_encode(const struct g_label_metadata *md, u_char *data)
{

	bcopy(md->md_magic, data, sizeof(md->md_magic));
	le32enc(data + 16, md->md_version);
	bcopy(md->md_label, data + 20, sizeof(md->md_label));
}
static __inline void
label_metadata_decode(const u_char *data, struct g_label_metadata *md)
{

	bcopy(data, md->md_magic, sizeof(md->md_magic));
	md->md_version = le32dec(data + 16);
	bcopy(data + 20, md->md_label, sizeof(md->md_label));
}
#endif	/* _G_LABEL_H_ */
