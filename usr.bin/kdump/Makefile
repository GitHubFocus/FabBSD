#	$OpenBSD: Makefile,v 1.16 2005/07/31 10:11:19 deraadt Exp $

LDSTATIC=${STATIC}
PROG=	kdump
CFLAGS+=-I${.CURDIR}/../ktrace -I${.CURDIR} -I${.CURDIR}/../../sys
SRCS=	kdump.c ioctl.c subr.c
.PATH:	${.CURDIR}/../ktrace
CLEANFILES+=ioctl.c

ioctl.c: ${.CURDIR}/Makefile ${.CURDIR}/mkioctls
	CC="${CC}" DESTDIR="${DESTDIR}" \
	/bin/sh ${.CURDIR}/mkioctls ${DESTDIR}/usr/include/sys/ioctl.h \
	    ${DESTDIR}/usr/include/crypto/cryptodev.h \
	    ${DESTDIR}/usr/include/dev/ccdvar.h \
	    ${DESTDIR}/usr/include/dev/ramdisk.h \
	    ${DESTDIR}/usr/include/dev/rndioctl.h \
	    ${DESTDIR}/usr/include/dev/wscons/wsconsio.h \
	    ${DESTDIR}/usr/include/dev/vndioctl.h \
	    ${DESTDIR}/usr/include/net/if_tun.h \
	    ${DESTDIR}/usr/include/net80211/ieee80211_ioctl.h \
	    ${DESTDIR}/usr/include/netinet6/in6_var.h \
	    ${DESTDIR}/usr/include/sys/agpio.h \
	    ${DESTDIR}/usr/include/sys/ataio.h \
	    ${DESTDIR}/usr/include/sys/cdio.h \
	    ${DESTDIR}/usr/include/sys/chio.h \
	    ${DESTDIR}/usr/include/sys/dkio.h \
	    ${DESTDIR}/usr/include/sys/filio.h \
	    ${DESTDIR}/usr/include/sys/gpio.h \
	    ${DESTDIR}/usr/include/sys/mtio.h \
	    ${DESTDIR}/usr/include/sys/pciio.h \
	    ${DESTDIR}/usr/include/sys/scanio.h \
	    ${DESTDIR}/usr/include/sys/scsiio.h \
	    ${DESTDIR}/usr/include/sys/sockio.h > ioctl.c

.include <bsd.prog.mk>
