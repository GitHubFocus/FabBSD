#	$OpenBSD: Makefile,v 1.37 2006/12/28 13:55:53 niallo Exp $

.PATH: ${.CURDIR}/..

PROG=	rcs
MAN=	ci.1 co.1 ident.1 merge.1 rcs.1 rcsclean.1 rcsdiff.1 rcsmerge.1 rlog.1

SRCS=	ci.c co.c ident.c merge.c rcsclean.c rcsdiff.c rcsmerge.c rcsprog.c \
	rlog.c rcsutil.c buf.c date.y diff.c diff3.c rcs.c rcsnum.c \
	rcstime.c worklist.c xmalloc.c

LINKS=	${BINDIR}/rcs ${BINDIR}/ci ${BINDIR}/rcs ${BINDIR}/co \
	${BINDIR}/rcs ${BINDIR}/ident ${BINDIR}/rcs ${BINDIR}/merge \
	${BINDIR}/rcs ${BINDIR}/rcsclean ${BINDIR}/rcs ${BINDIR}/rcsdiff \
	${BINDIR}/rcs ${BINDIR}/rcsmerge ${BINDIR}/rcs ${BINDIR}/rlog

CPPFLAGS+=-I${.CURDIR}
CFLAGS+=-Wall
CFLAGS+=-Wstrict-prototypes -Wmissing-prototypes
CFLAGS+=-Wmissing-declarations
CFLAGS+=-Wshadow -Wpointer-arith -Wcast-qual
CFLAGS+=-Wsign-compare
DEBUG=	-g -ggdb
YFLAGS=

.include <bsd.prog.mk>
