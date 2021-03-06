/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char *rcsid = "$OpenBSD: htons.c,v 1.6 2007/11/24 19:25:02 deraadt Exp $";
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <machine/endian.h>

#undef htons

u_int16_t		htons(u_int16_t);

u_int16_t
htons(u_int16_t x)
{
#if BYTE_ORDER == LITTLE_ENDIAN
	u_char *s = (u_char *) &x;
	return (u_int16_t)(s[0] << 8 | s[1]);
#else
	return x;
#endif
}
