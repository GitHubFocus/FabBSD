#	$FabBSD$
#	$OpenBSD: Makefile,v 1.5 2008/03/04 05:37:51 deraadt Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

CDROM=		install${REV}.iso
RELXDIR=	/home/relx-${MACHINE}
RELDIR?=	/home/rel-${MACHINE}

BASE=		${RELDIR}/base${OSrev}.tgz ${RELDIR}/comp${OSrev}.tgz \
		${RELDIR}/etc${OSrev}.tgz ${RELDIR}/man${OSrev}.tgz \
		${RELDIR}/misc${OSrev}.tgz \
		${RELDIR}/bsd ${RELDIR}/bsd.mp ${RELDIR}/bsd.rd \
		${RELDIR}/INSTALL.${MACHINE}

all: ${CDROM}

${CDROM}: ${BASE}
	-rm -rf ${.OBJDIR}/cd-dir
	-mkdir -p ${.OBJDIR}/cd-dir/${OSREV}/${MACHINE}

	cp -p ${BASE} ${.OBJDIR}/cd-dir/${OSREV}/${MACHINE}

	cp -p ${RELDIR}/ofwboot ${.OBJDIR}/cd-dir/${OSREV}/${MACHINE}

	echo 'm 1	sun4' > ${.OBJDIR}/cd-dir/.slicemapfile
	echo 'm 2	sun4c' >> ${.OBJDIR}/cd-dir/.slicemapfile
	echo 'm 3	sun4m' >> ${.OBJDIR}/cd-dir/.slicemapfile
	echo 'm 4	sun4d' >> ${.OBJDIR}/cd-dir/.slicemapfile
	echo 'm 5	sun4u' >> ${.OBJDIR}/cd-dir/.slicemapfile
	echo 'm 5	sun4us' >> ${.OBJDIR}/cd-dir/.slicemapfile
	echo 'm 6	sun4v' >> ${.OBJDIR}/cd-dir/.slicemapfile
	mkhybrid -R -T -L -d -D -N \
	    -A "FabBSD ${OSREV} ${MACHINE} Install CD" \
	    -P "Copyright (c) `date +%Y` Hypertriton, Inc." \
	    -p "Julien Nadeau <vedge@hypertriton.com>" \
	    -V "FabBSD/${MACHINE} ${OSREV} Install CD" \
	    -o ${.OBJDIR}/${CDROM} ${.OBJDIR}/cd-dir
	vnconfig -v -c svnd0 ${.OBJDIR}/${CDROM}
	disklabel -w svnd0 fakecdrom "FabBSD${REV}${MACHINE}"
	vnconfig -u svnd0
	mksuncd f ${.OBJDIR}/${CDROM} ${.OBJDIR}/../miniroot/miniroot${REV}.fs

install:
	cp ${CDROM} ${RELDIR}/

clean cleandir:
	/bin/rm -rf ${CDROM} ${.OBJDIR}/cd-dir

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
