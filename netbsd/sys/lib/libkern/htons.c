/*	$NetBSD: htons.c,v 1.9 1998/03/27 01:30:02 cgd Exp $	*/

/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: htons.c,v 1.9 1998/03/27 01:30:02 cgd Exp $");
#endif

#include <sys/types.h>

#undef htons

in_port_t
htons(x)
	in_port_t x;
{
#if BYTE_ORDER == LITTLE_ENDIAN
	u_char *s = (u_char *) &x;
	return (in_port_t)(s[0] << 8 | s[1]);
#else
	return x;
#endif
}
