#	$FabBSD$
#	$OpenBSD: Makefile,v 1.3 2005/03/11 15:40:58 deraadt Exp $

TOP=	${.CURDIR}/..

.include "${TOP}/Makefile.inc"

CDFS=	cdrom${REV}.fs
CDROM=	cdemu${REV}.iso

all: ${CDROM}

${CDROM}:
	-rm -rf ${.OBJDIR}/cd-dir
	mkdir -p ${.OBJDIR}/cd-dir/${OSREV}/amd64
	cp ${.OBJDIR}/../ramdisk_cd/${CDFS} ${.OBJDIR}/cd-dir/${OSREV}/amd64
	mkhybrid -a -R -T -L -d -D -N -o ${.OBJDIR}/${CDROM} -v -v \
	    -A "FabBSD ${OSREV} amd64 bootonly CD" \
	    -P "Copyright (c) `date +%Y` Hypertriton, Inc." \
	    -p "Julien Nadeau <vedge@hypertriton.com>" \
	    -V "FabBSD/amd64   ${OSREV} boot-only CD" \
	    -b ${OSREV}/amd64/${CDFS} -c ${OSREV}/amd64/boot.catalog \
	    ${.OBJDIR}/cd-dir

install:
	cp ${CDROM} ${DESTDIR}/snapshot

clean cleandir:
	/bin/rm -f ${CDROM}
	rm -rf cd-dir

.include <bsd.obj.mk>
.include <bsd.subdir.mk>
