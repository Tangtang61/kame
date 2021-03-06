/*
 * Copyright 2002 by Peter Grehan. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/powerpc/powermac/ata_macio.c,v 1.1 2002/09/19 04:52:07 grehan Exp $
 */

/*
 * Mac-io ATA controller
 */
#include "opt_ata.h"
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/disk.h>
#include <sys/module.h>
#include <sys/bus.h>
#include <sys/bio.h>
#include <sys/malloc.h>
#include <sys/devicestat.h>
#include <sys/sysctl.h>
#include <machine/stdarg.h>
#include <machine/resource.h>
#include <machine/bus.h>
#include <sys/rman.h>
#include <dev/ata/ata-all.h>

#include <dev/ofw/openfirm.h>
#include <powerpc/powermac/maciovar.h>

/*
 * Define the macio ata bus attachment. This creates a pseudo-bus that
 * the ATA device can be attached to
 */
static  int  ata_macio_attach(device_t dev);
static  int  ata_macio_probe(device_t dev);
static  int  ata_macio_print_child(device_t dev, device_t child);
struct resource *ata_macio_alloc_resource(device_t, device_t, int, int *,
					  u_long, u_long, u_long, u_int);
static int ata_macio_release_resource(device_t, device_t, int, int,
				      struct resource *);

static device_method_t ata_macio_methods[] = {
        /* Device interface */
	DEVMETHOD(device_probe,		ata_macio_probe),
	DEVMETHOD(device_attach,        ata_macio_attach),
	DEVMETHOD(device_shutdown,      bus_generic_shutdown),
	DEVMETHOD(device_suspend,       bus_generic_suspend),
	DEVMETHOD(device_resume,        bus_generic_resume),

	/* Bus methods */
	DEVMETHOD(bus_print_child,          ata_macio_print_child),
	DEVMETHOD(bus_alloc_resource,       ata_macio_alloc_resource),
	DEVMETHOD(bus_release_resource,     ata_macio_release_resource),
	DEVMETHOD(bus_activate_resource,    bus_generic_activate_resource),
	DEVMETHOD(bus_deactivate_resource,  bus_generic_deactivate_resource),
	DEVMETHOD(bus_setup_intr,           bus_generic_setup_intr),
	DEVMETHOD(bus_teardown_intr,        bus_generic_teardown_intr),

	{ 0, 0 }
};

static driver_t ata_macio_driver = {
	"atamacio",
	ata_macio_methods,
	0,
};

static devclass_t ata_macio_devclass;

DRIVER_MODULE(atamacio, macio, ata_macio_driver, ata_macio_devclass, 0, 0);


static int
ata_macio_probe(device_t dev)
{
	char *type = macio_get_devtype(dev);

	if (strcmp(type, "ata") != 0)
		return (ENXIO);

	/* Print keylargo/pangea ??? */
	device_set_desc(dev, "Mac-IO ATA Controller");
	return (0);	
}


static int
ata_macio_attach(device_t dev)
{
	/*
	 * Add a single child per controller. Should be able
	 * to add two
	 */
	device_add_child(dev, "ata", 
			 devclass_find_free_unit(ata_devclass, 0));

	return (bus_generic_attach(dev));
}


static int
ata_macio_print_child(device_t dev, device_t child)
{
    int retval = 0;

    retval += bus_print_child_header(dev, child);
    retval += bus_print_child_footer(dev, child);

    return (retval);
}

/* offset to control registers from base */
#define ATA_MACIO_ALTOFFSET 0x160

struct resource *
ata_macio_alloc_resource(device_t dev, device_t child, int type, int *rid,
			 u_long start, u_long end, u_long count, u_int flags)
{
	struct resource *res = NULL;
	int myrid;
	u_int *ofw_regs;

	ofw_regs = macio_get_regs(dev);

	/*
	 * The offset for the register bank is in the first ofw register,
	 * with the base address coming from the parent macio bus (but
	 * accessible via the ata-macio child)
	 */
	if (type == SYS_RES_IOPORT) {
		switch (*rid) {
		case ATA_IOADDR_RID:
			myrid = 0;
			start = ofw_regs[0];
			end = start + (ATA_IOSIZE << 4) - 1;
			count = ATA_IOSIZE << 4;
			res = BUS_ALLOC_RESOURCE(device_get_parent(dev), child,
						 SYS_RES_IOPORT, &myrid,
						 start, end, count,
						 PPC_BUS_SPARSE4 | flags);
			break;

		case ATA_ALTADDR_RID:		
			myrid = 0;
			start = ofw_regs[0] + ATA_MACIO_ALTOFFSET;
			end = start + (ATA_ALTIOSIZE << 4) - 1;
			count = ATA_ALTIOSIZE << 4;
			res = BUS_ALLOC_RESOURCE(device_get_parent(dev), child,
						 SYS_RES_IOPORT, &myrid,
						 start, end, count,
						 PPC_BUS_SPARSE4 | flags);
			break;

		case ATA_BMADDR_RID:
			/* looks difficult to support DBDMA in FreeBSD... */
			break;
		}
		return (res);

	} else if (type == SYS_RES_IRQ && *rid == ATA_IRQ_RID) {
		/*
		 * Pass this on to the parent, using the IRQ from the
		 * ATA pseudo-bus resource
		 */
		res = bus_generic_rl_alloc_resource(device_get_parent(dev),
		   dev, SYS_RES_IRQ, 0, 0, ~0, 1, flags);
		return (res);

	} else {	
		return (NULL);
	}
}


static int
ata_macio_release_resource(device_t dev, device_t child, int type, int rid,
			   struct resource *r)
{
	printf("macio: release resource\n");
	return (0);
}


/*
 * Define the actual ATA device. This is a sub-bus to the ata-macio layer
 * to allow the higher layer bus to massage the resource allocation.
 */

static  int  ata_macio_sub_probe(device_t dev);

static device_method_t ata_macio_sub_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe,     ata_macio_sub_probe),
	DEVMETHOD(device_attach,    ata_attach),
	DEVMETHOD(device_detach,    ata_detach),
	DEVMETHOD(device_resume,    ata_resume),
	
	{ 0, 0 }
};

static driver_t ata_macio_sub_driver = {
	"ata",
	ata_macio_sub_methods,
	sizeof(struct ata_channel),
};

DRIVER_MODULE(ata, atamacio, ata_macio_sub_driver, ata_devclass, 0, 0);

static int
ata_macio_sub_probe(device_t dev)
{
	struct ata_channel *ch = device_get_softc(dev);

	ch->unit = 0; 
	ch->flags = ATA_USE_16BIT;

	return ata_probe(dev);
}
