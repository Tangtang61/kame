/*	$OpenBSD: dzkbdvar.h,v 1.1 2001/05/16 22:15:17 hugh Exp $	*/
/* $NetBSD: dzkbdvar.h,v 1.2 2001/03/06 07:40:52 matt Exp $ */

struct dzkm_attach_args {
	int	daa_line;	/* Line to search */
	int	daa_flags;	/* Console etc...*/
#define	DZKBD_CONSOLE	1
};



/* These functions must be present for the keyboard/mouse to work */
int dzgetc(struct dz_linestate *);
void dzputc(struct dz_linestate *, int);
void dzsetlpr(struct dz_linestate *, int);

/* Exported functions */
int dzkbd_cnattach(struct dz_linestate *);
