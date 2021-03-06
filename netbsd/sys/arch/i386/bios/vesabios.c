/* $NetBSD: vesabios.c,v 1.10 2003/07/14 22:13:09 lukem Exp $ */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vesabios.c,v 1.10 2003/07/14 22:13:09 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <machine/frame.h>
#include <machine/kvm86.h>

#include <arch/i386/bios/vesabios.h>
#include <arch/i386/bios/vesabiosreg.h>

#include "opt_vesabios.h"

struct vbeinfoblock
{
	char VbeSignature[4];
	u_int16_t VbeVersion;
	u_int32_t OemStringPtr;
	u_int32_t Capabilities;
	u_int32_t VideoModePtr;
	u_int16_t TotalMemory;
	u_int16_t OemSoftwareRev;
	u_int32_t OemVendorNamePtr, OemProductNamePtr, OemProductRevPtr;
	/* data area, in total max 512 bytes for VBE 2.0 */
} __attribute__ ((packed));

#define FAR2FLATPTR(p) ((p & 0xffff) + ((p >> 12) & 0xffff0))

static int vesabios_match(struct device *, struct cfdata *, void *);
static void vesabios_attach(struct device *, struct device *, void *);
static int vesabios_print(void *, const char *);

static int vbegetinfo(struct vbeinfoblock **);
static void vbefreeinfo(struct vbeinfoblock *);
#ifdef VESABIOSVERBOSE
static const char *mm2txt(unsigned int);
#endif

CFATTACH_DECL(vesabios, sizeof(struct device),
    vesabios_match, vesabios_attach, NULL, NULL);

static int
vesabios_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct vesabios_attach_args *vaa = aux;

	/* These are not the droids you're looking for. */
	if (strcmp(vaa->vaa_busname, "vesabios") != 0)
		return (0);

	return (1);
}

static int
vbegetinfo(vip)
	struct vbeinfoblock **vip;
{
	unsigned char *buf;
	struct trapframe tf;
	int res, error;

	buf = kvm86_bios_addpage(0x2000);
	if (!buf) {
		printf("vbegetinfo: kvm86_bios_addpage(0x2000) failed\n");
		return (ENOMEM);
	}

	memcpy(buf, "VBE2", 4);

	memset(&tf, 0, sizeof(struct trapframe));
	tf.tf_eax = 0x4f00; /* function code */
	tf.tf_vm86_es = 0;
	tf.tf_edi = 0x2000; /* buf ptr */

	res = kvm86_bioscall(0x10, &tf);
	if (res || tf.tf_eax != 0x004f) {
		printf("vbecall: res=%d, ax=%x\n", res, tf.tf_eax);
		error = ENXIO;
		goto out;
	}

	if (memcmp(((struct vbeinfoblock *)buf)->VbeSignature, "VESA", 4)) {
		error = EIO;
		goto out;
	}

	if (vip)
		*vip = (struct vbeinfoblock *)buf;
	return (0);

out:
	kvm86_bios_delpage(0x2000, buf);
	return (error);
}

static void
vbefreeinfo(vip)
	struct vbeinfoblock *vip;
{

	kvm86_bios_delpage(0x2000, vip);
}

int
vbeprobe()
{
	struct vbeinfoblock *vi;

	if (vbegetinfo(&vi))
		return (0);
	vbefreeinfo(vi);
	return (1);
}

#ifdef VESABIOSVERBOSE
static const char *
mm2txt(mm)
	unsigned int mm;
{
	static char buf[30];
	static const char *names[] = {
		"Text mode",
		"CGA graphics",
		"Hercules graphics",
		"Planar",
		"Packed pixel",
		"Non-chain 4, 256 color",
		"Direct Color",
		"YUV"
	};

	if (mm < sizeof(names)/sizeof(names[0]))
		return (names[mm]);
	snprintf(buf, sizeof(buf), "unknown memory model %d", mm);
	return (buf);
}
#endif

static void
vesabios_attach(parent, dev, aux)
	struct device * parent, *dev;
	void *aux;
{
	struct vbeinfoblock *vi;
	unsigned char *buf;
	struct trapframe tf;
	int res;
	char name[256];
#define MAXMODES 60
	u_int16_t modes[MAXMODES];
	int raster8modes[MAXMODES];
	int textmodes[MAXMODES];
	int nmodes, nraster8modes, ntextmodes, i;
	u_int32_t modeptr;
	struct modeinfoblock *mi;
	struct vesabiosdev_attach_args vbaa;

	if (vbegetinfo(&vi))
		panic("vesabios_attach: disappeared");

	printf(": version %d.%d", vi->VbeVersion >> 8, vi->VbeVersion & 0xff);
	
	res = kvm86_bios_read(FAR2FLATPTR(vi->OemVendorNamePtr),
			      name, sizeof(name));
	if (res > 0) {
		name[res - 1] = 0;
		printf(", %s", name);
		res = kvm86_bios_read(FAR2FLATPTR(vi->OemProductNamePtr),
				      name, sizeof(name));
		if (res > 0) {
			name[res - 1] = 0;
			printf(" %s", name);
		}
	}
	printf("\n");

	nmodes = 0;
	modeptr = FAR2FLATPTR(vi->VideoModePtr);
	while (nmodes < MAXMODES) {
		res = kvm86_bios_read(modeptr, (char *)&modes[nmodes], 2);
		if (res != 2 || modes[nmodes] == 0xffff)
			break;
		nmodes++;
		modeptr += 2;
	}

	vbefreeinfo(vi);
	if (nmodes == 0)
		return;

	nraster8modes = ntextmodes = 0;

	buf = kvm86_bios_addpage(0x2000);
	if (!buf) {
		printf("vesabios_attach: kvm86_bios_addpage(0x2000) failed\n");
		return;
	}
	for (i = 0; i < nmodes; i++) {

		memset(&tf, 0, sizeof(struct trapframe));
		tf.tf_eax = 0x4f01; /* function code */
		tf.tf_ecx = modes[i];
		tf.tf_vm86_es = 0;
		tf.tf_edi = 0x2000; /* buf ptr */

		res = kvm86_bioscall(0x10, &tf);
		if (res || tf.tf_eax != 0x004f) {
			printf("vbecall: res=%d, ax=%x\n", res, tf.tf_eax);
			printf("error getting info for mode %04x\n", modes[i]);
			continue;
		}
		mi = (struct modeinfoblock *)buf;
#ifdef VESABIOSVERBOSE
		printf("VESA mode %04x: attributes %04x",
		       modes[i], mi->ModeAttributes);
#endif
		if (!(mi->ModeAttributes & 1)) {
#ifdef VESABIOSVERBOSE
			printf("\n");
#endif
			continue;
		}
		if (mi->ModeAttributes & 0x10) {
			/* graphics */
#ifdef VESABIOSVERBOSE
			printf(", %dx%d %dbbp %s\n",
			       mi->XResolution, mi->YResolution,
			       mi->BitsPerPixel, mm2txt(mi->MemoryModel));
#endif
			if ((mi->ModeAttributes & 0x80)
			    && mi->BitsPerPixel == 8
			    && mi->MemoryModel == 4) {
				/* flat buffer, 8bpp packed pixel */
				raster8modes[nraster8modes++] = modes[i];
			}
		} else {
			/* text */
#ifdef VESABIOSVERBOSE
			printf(", text %dx%d\n",
			       mi->XResolution, mi->YResolution);
#endif
			if (!(mi->ModeAttributes & 0x20)) /* VGA compatible */
				textmodes[ntextmodes++] = modes[i];
		}
	}
	kvm86_bios_delpage(0x2000, buf);

	if (nraster8modes) {
		vbaa.vbaa_type = "raster8";
		vbaa.vbaa_modes = raster8modes;
		vbaa.vbaa_nmodes = nraster8modes;

		config_found(dev, &vbaa, vesabios_print);
	}
	if (ntextmodes) {
		vbaa.vbaa_type = "text";
		vbaa.vbaa_modes = textmodes;
		vbaa.vbaa_nmodes = ntextmodes;

		config_found(dev, &vbaa, vesabios_print);
	}
}

static int
vesabios_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct vesabiosdev_attach_args *vbaa = aux;

	if (pnp)
		aprint_normal("%s at %s", vbaa->vbaa_type, pnp);
	return (UNCONF);
}
