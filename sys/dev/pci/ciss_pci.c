/*	$OpenBSD: ciss_pci.c,v 1.12 2008/06/11 20:24:47 jsg Exp $	*/

/*
 * Copyright (c) 2005 Michael Shalayeff
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcivar.h>

#include <machine/bus.h>

#include <scsi/scsi_all.h>
#include <scsi/scsi_disk.h>
#include <scsi/scsiconf.h>

#include <dev/ic/cissreg.h>
#include <dev/ic/cissvar.h>

#define	CISS_BAR	0x10

int	ciss_pci_match(struct device *, void *, void *);
void	ciss_pci_attach(struct device *, struct device *, void *);

struct cfattach ciss_pci_ca = {
	sizeof(struct ciss_softc), ciss_pci_match, ciss_pci_attach
};

const struct pci_matchid ciss_pci_devices[] = {
	{ PCI_VENDOR_COMPAQ,	PCI_PRODUCT_COMPAQ_CSA532 },
	{ PCI_VENDOR_COMPAQ,	PCI_PRODUCT_COMPAQ_CSA5300 },
	{ PCI_VENDOR_COMPAQ,	PCI_PRODUCT_COMPAQ_CSA5300_2 },
	{ PCI_VENDOR_COMPAQ,	PCI_PRODUCT_COMPAQ_CSA5312 },
	{ PCI_VENDOR_COMPAQ,	PCI_PRODUCT_COMPAQ_CSA5i },
	{ PCI_VENDOR_COMPAQ,	PCI_PRODUCT_COMPAQ_CSA5i_2 },
	{ PCI_VENDOR_COMPAQ,	PCI_PRODUCT_COMPAQ_CSA6i },
	{ PCI_VENDOR_COMPAQ,	PCI_PRODUCT_COMPAQ_CSA641 },
	{ PCI_VENDOR_COMPAQ,	PCI_PRODUCT_COMPAQ_CSA642 },
	{ PCI_VENDOR_COMPAQ,	PCI_PRODUCT_COMPAQ_CSA6400 },
	{ PCI_VENDOR_COMPAQ,	PCI_PRODUCT_COMPAQ_CSA6400EM },
	{ PCI_VENDOR_COMPAQ,	PCI_PRODUCT_COMPAQ_CSA6422 },
	{ PCI_VENDOR_COMPAQ,	PCI_PRODUCT_COMPAQ_CSA64XX },
	{ PCI_VENDOR_HP,	PCI_PRODUCT_HP_HPSAE200 },
	{ PCI_VENDOR_HP,	PCI_PRODUCT_HP_HPSAE200I_1 },
	{ PCI_VENDOR_HP,	PCI_PRODUCT_HP_HPSAE200I_2 },
	{ PCI_VENDOR_HP,	PCI_PRODUCT_HP_HPSAE200I_3 },
	{ PCI_VENDOR_HP,	PCI_PRODUCT_HP_HPSAE500 },
	{ PCI_VENDOR_HP,	PCI_PRODUCT_HP_HPSAP212 },
	{ PCI_VENDOR_HP,	PCI_PRODUCT_HP_HPSAP410 },
	{ PCI_VENDOR_HP,	PCI_PRODUCT_HP_HPSAP410I },
	{ PCI_VENDOR_HP,	PCI_PRODUCT_HP_HPSAP411 },
	{ PCI_VENDOR_HP,	PCI_PRODUCT_HP_HPSAP600 },
	{ PCI_VENDOR_HP,	PCI_PRODUCT_HP_HPSAP800 },
	{ PCI_VENDOR_HP,	PCI_PRODUCT_HP_HPSAP812 },
	{ PCI_VENDOR_HP,	PCI_PRODUCT_HP_HPSAV100 },
	{ PCI_VENDOR_HP,	PCI_PRODUCT_HP_HPSA_1 },
	{ PCI_VENDOR_HP,	PCI_PRODUCT_HP_HPSA_2 },
	{ PCI_VENDOR_HP,	PCI_PRODUCT_HP_HPSA_3 },
	{ PCI_VENDOR_HP,	PCI_PRODUCT_HP_HPSA_4 },
	{ PCI_VENDOR_HP,	PCI_PRODUCT_HP_HPSA_5 },
	{ PCI_VENDOR_HP,	PCI_PRODUCT_HP_HPSA_6 },
	{ PCI_VENDOR_HP,	PCI_PRODUCT_HP_HPSA_7 },
	{ PCI_VENDOR_HP,	PCI_PRODUCT_HP_HPSA_8 },
	{ PCI_VENDOR_HP,	PCI_PRODUCT_HP_HPSA_9 },
	{ PCI_VENDOR_HP,	PCI_PRODUCT_HP_HPSA_10 },
	{ PCI_VENDOR_HP,	PCI_PRODUCT_HP_HPSA_11 },
	{ PCI_VENDOR_HP,	PCI_PRODUCT_HP_HPSA_12 }
};
#define	CISS_PCI_NDEVS	sizeof(ciss_pci_devices)/sizeof(ciss_pci_devices[0])

int
ciss_pci_match(struct device *parent, void *match, void *aux)
{
	struct pci_attach_args *pa = aux;

	return pci_matchbyid(pa, ciss_pci_devices, CISS_PCI_NDEVS);
}

void
ciss_pci_attach(struct device *parent, struct device *self, void *aux)
{
	struct ciss_softc *sc = (struct ciss_softc *)self;
	struct pci_attach_args *pa = aux;
	bus_size_t size, cfgsz;
	pci_intr_handle_t ih;
	const char *intrstr;
	int cfg_bar, memtype;
	pcireg_t reg;

	memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, CISS_BAR);
	if (pci_mapreg_map(pa, CISS_BAR, memtype, 0,
	    &sc->iot, &sc->ioh, NULL, &size, 0)) {
		printf(": can't map controller mem space\n");
		return;
	}
	sc->dmat = pa->pa_dmat;

	sc->iem = CISS_READYENA;
	reg = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_SUBSYS_ID_REG);
	if (PCI_VENDOR(reg) == PCI_VENDOR_COMPAQ &&
	    (PCI_PRODUCT(reg) == PCI_PRODUCT_COMPAQ_CSA5i ||
	     PCI_PRODUCT(reg) == PCI_PRODUCT_COMPAQ_CSA532 ||
	     PCI_PRODUCT(reg) == PCI_PRODUCT_COMPAQ_CSA5312))
		sc->iem = CISS_READYENAB;

	cfg_bar = bus_space_read_2(sc->iot, sc->ioh, CISS_CFG_BAR);
	sc->cfgoff = bus_space_read_4(sc->iot, sc->ioh, CISS_CFG_OFF);
	if (cfg_bar != CISS_BAR) {
		if (pci_mapreg_map(pa, cfg_bar, PCI_MAPREG_TYPE_MEM, 0,
		    NULL, &sc->cfg_ioh, NULL, &cfgsz, 0)) {
			printf(": can't map controller config space\n");  
			bus_space_unmap(sc->iot, sc->ioh, size);
			return;
		}
	} else {
		sc->cfg_ioh = sc->ioh;
		cfgsz = size;
	}

	if (sc->cfgoff + sizeof(struct ciss_config) > cfgsz) {
		printf(": unfit config space\n");
		bus_space_unmap(sc->iot, sc->ioh, size);
		if (cfg_bar != CISS_BAR)
			bus_space_unmap(sc->iot, sc->cfg_ioh, cfgsz);
		return;
	}

	/* disable interrupts until ready */
	bus_space_write_4(sc->iot, sc->ioh, CISS_IMR,
	    bus_space_read_4(sc->iot, sc->ioh, CISS_IMR) | sc->iem);

	if (pci_intr_map(pa, &ih)) {
		printf(": can't map interrupt\n");
		bus_space_unmap(sc->iot, sc->ioh, size);
		if (cfg_bar != CISS_BAR)
			bus_space_unmap(sc->iot, sc->cfg_ioh, cfgsz);
		return;
	}
	intrstr = pci_intr_string(pa->pa_pc, ih);
	sc->sc_ih = pci_intr_establish(pa->pa_pc, ih, IPL_BIO, ciss_intr, sc,
	    sc->sc_dev.dv_xname);
	if (!sc->sc_ih) {
		printf(": can't establish interrupt");
		if (intrstr)
			printf(" at %s", intrstr);
		printf("\n");
		bus_space_unmap(sc->iot, sc->ioh, size);
		if (cfg_bar != CISS_BAR)
			bus_space_unmap(sc->iot, sc->cfg_ioh, cfgsz);
	}

	printf(": %s\n%s", intrstr, sc->sc_dev.dv_xname);

	if (ciss_attach(sc)) {
		pci_intr_disestablish(pa->pa_pc, sc->sc_ih);
		sc->sc_ih = NULL;
		bus_space_unmap(sc->iot, sc->ioh, size);
		if (cfg_bar != CISS_BAR)
			bus_space_unmap(sc->iot, sc->cfg_ioh, cfgsz);
		return;
	}

	/* enable interrupts now */
	bus_space_write_4(sc->iot, sc->ioh, CISS_IMR,
	    bus_space_read_4(sc->iot, sc->ioh, CISS_IMR) & ~sc->iem);
}
