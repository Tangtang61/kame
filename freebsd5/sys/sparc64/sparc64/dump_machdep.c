/*
 * Copyright (c) 2002 Marcel Moolenaar
 * Copyright (c) 2002 Thomas Moestl
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/sparc64/sparc64/dump_machdep.c,v 1.2 2002/10/20 17:03:15 tmm Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/cons.h>
#include <sys/kernel.h>
#include <sys/kerneldump.h>

#include <vm/vm.h>
#include <vm/pmap.h>

#include <machine/metadata.h>
#include <machine/kerneldump.h>
#include <machine/ofw_mem.h>
#include <machine/tsb.h>

CTASSERT(sizeof(struct kerneldumpheader) == DEV_BSIZE);

extern struct ofw_mem_region sparc64_memreg[];
extern int sparc64_nmemreg;

static struct kerneldumpheader kdh;
static off_t dumplo, dumppos;

/* Handle buffered writes. */
static char buffer[DEV_BSIZE];
static vm_size_t fragsz;

#define	MAXDUMPSZ	(MAXDUMPPGS << PAGE_SHIFT)

/* XXX should be MI */
static void
mkdumpheader(struct kerneldumpheader *kdh, uint32_t archver, uint64_t dumplen,
    uint32_t blksz)
{

	bzero(kdh, sizeof(*kdh));
	strncpy(kdh->magic, KERNELDUMPMAGIC, sizeof(kdh->magic));
	strncpy(kdh->architecture, MACHINE_ARCH, sizeof(kdh->architecture));
	kdh->version = htod32(KERNELDUMPVERSION);
	kdh->architectureversion = htod32(archver);
	kdh->dumplength = htod64(dumplen);
	kdh->dumptime = htod64(time_second);
	kdh->blocksize = htod32(blksz);
	strncpy(kdh->hostname, hostname, sizeof(kdh->hostname));
	strncpy(kdh->versionstring, version, sizeof(kdh->versionstring));
	if (panicstr != NULL)
		strncpy(kdh->panicstring, panicstr, sizeof(kdh->panicstring));
	kdh->parity = kerneldump_parity(kdh);
}

static int
buf_write(struct dumperinfo *di, char *ptr, size_t sz)
{
	size_t len;
	int error;

	while (sz) {
		len = DEV_BSIZE - fragsz;
		if (len > sz)
			len = sz;
		bcopy(ptr, buffer + fragsz, len);
		fragsz += len;
		ptr += len;
		sz -= len;
		if (fragsz == DEV_BSIZE) {
			error = di->dumper(di->priv, buffer, 0, dumplo,
			    DEV_BSIZE);
			if (error)
				return error;
			dumplo += DEV_BSIZE;
			fragsz = 0;
		}
	}

	return (0);
}

static int
buf_flush(struct dumperinfo *di)
{
	int error;

	if (fragsz == 0)
		return (0);

	error = di->dumper(di->priv, buffer, 0, dumplo, DEV_BSIZE);
	dumplo += DEV_BSIZE;
	return (error);
}

static int
reg_write(struct dumperinfo *di, vm_offset_t pa, vm_size_t size)
{
	struct sparc64_dump_reg r;

	r.dr_pa = pa;
	r.dr_size = size;
	r.dr_offs = dumppos;
	dumppos += size;
	return (buf_write(di, (char *)&r, sizeof(r)));
}

static int
blk_dump(struct dumperinfo *di, vm_offset_t pa, vm_size_t size)
{
	vm_size_t pos, npg, rsz;
	void *va;
	int c, counter, error, i, twiddle;

	printf("  chunk at %#lx: %ld bytes ", (u_long)pa, (long)size);

	va = NULL;
	error = counter = twiddle = 0;
	for (pos = 0; pos < size; pos += MAXDUMPSZ, counter++) {
		if (counter % 128 == 0)
			printf("%c\b", "|/-\\"[twiddle++ & 3]);
		rsz = size - pos;
		rsz = (rsz > MAXDUMPSZ) ? MAXDUMPSZ : rsz;
		npg = rsz >> PAGE_SHIFT;
		for (i = 0; i < npg; i++)
			va = pmap_kenter_temporary(pa + pos + i * PAGE_SIZE, i);
		error = di->dumper(di->priv, va, 0, dumplo, rsz);
		if (error)
			break;
		dumplo += rsz;

		/* Check for user abort. */
		c = cncheckc();
		if (c == 0x03)
			return (ECANCELED);
		if (c != -1)
			printf("(CTRL-C to abort)  ");
	}
	printf("... %s\n", (error) ? "fail" : "ok");
	return (error);
}

void
dumpsys(struct dumperinfo *di)
{
	struct sparc64_dump_hdr hdr;
	vm_size_t size, totsize, hdrsize;
	int error, i, nreg;

	/* Calculate dump size. */
	size = 0;
	nreg = sparc64_nmemreg;
	for (i = 0; i < sparc64_nmemreg; i++)
		size += sparc64_memreg[i].mr_size;
	/* Account for the header size. */
	hdrsize = roundup2(sizeof(hdr) + sizeof(struct sparc64_dump_reg) * nreg,
	    DEV_BSIZE);
	size += hdrsize;

	totsize = size + 2 * sizeof(kdh);
	if (totsize > di->mediasize) {
		printf("Insufficient space on device (need %ld, have %ld), "
		    "refusing to dump.\n", (long)totsize,
		    (long)di->mediasize);
		error = ENOSPC;
		goto fail;
	}

	/* Determine dump offset on device. */
	dumplo = di->mediaoffset + di->mediasize - totsize;

	mkdumpheader(&kdh, KERNELDUMP_SPARC64_VERSION, size, di->blocksize);

	printf("Dumping %lu MB (%d chunks)\n", (u_long)(size >> 20), nreg);

	/* Dump leader */
	error = di->dumper(di->priv, &kdh, 0, dumplo, sizeof(kdh));
	if (error)
		goto fail;
	dumplo += sizeof(kdh);

	/* Dump the private header. */
	hdr.dh_hdr_size = hdrsize;
	hdr.dh_tsb_pa = tsb_kernel_phys;
	hdr.dh_tsb_size = tsb_kernel_size;
	hdr.dh_tsb_mask = tsb_kernel_mask;
	hdr.dh_nregions = nreg;

	if (buf_write(di, (char *)&hdr, sizeof(hdr)) != 0)
		goto fail;

	dumppos = hdrsize;
	/* Now, write out the region descriptors. */
	for (i = 0; i < sparc64_nmemreg; i++) {
		error = reg_write(di, sparc64_memreg[i].mr_start,
		    sparc64_memreg[i].mr_size);
		if (error != 0)
			goto fail;
	}
	buf_flush(di);

	/* Dump memory chunks. */
	for (i = 0; i < sparc64_nmemreg; i++) {
		error = blk_dump(di, sparc64_memreg[i].mr_start,
		    sparc64_memreg[i].mr_size);
		if (error != 0)
			goto fail;
	}

	/* Dump trailer */
	error = di->dumper(di->priv, &kdh, 0, dumplo, sizeof(kdh));
	if (error)
		goto fail;

	/* Signal completion, signoff and exit stage left. */
	di->dumper(di->priv, NULL, 0, 0, 0);
	printf("\nDump complete\n");
	return;

 fail:
	/* XXX It should look more like VMS :-) */
	printf("** DUMP FAILED (ERROR %d) **\n", error);
}
