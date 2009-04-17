/*
 *	Low-Level PCI Support for the SH7780
 *
 *  Dustin McIntire (dustin@sensoria.com)
 *	Derived from arch/i386/kernel/pci-*.c which bore the message:
 *	(c) 1999--2000 Martin Mares <mj@ucw.cz>
 *
 *  Ported to the new API by Paul Mundt <lethal@linux-sh.org>
 *  With cleanup by Paul van Gool <pvangool@mimotech.com>
 *
 *  May be copied or modified under the terms of the GNU General Public
 *  License.  See linux/COPYING for more information.
 *
 */
#undef DEBUG

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include "pci-sh4.h"

/*
 * Initialization. Try all known PCI access methods. Note that we support
 * using both PCI BIOS and direct access: in such cases, we use I/O ports
 * to access config space.
 *
 * Note that the platform specific initialization (BSC registers, and memory
 * space mapping) will be called via the platform defined function
 * pcibios_init_platform().
 */
int __init sh7780_pci_init(struct pci_channel *chan)
{
	unsigned int id;
	int ret, match = 0;

	pr_debug("PCI: Starting intialization.\n");

	chan->reg_base = 0xfe040000;
	chan->io_base = 0xfe200000;

	ctrl_outl(0x00000001, SH7780_PCI_VCR2); /* Enable PCIC */

	/* check for SH7780/SH7780R hardware */
	id = pci_read_reg(chan, SH7780_PCIVID);
	if ((id & 0xffff) == SH7780_VENDOR_ID) {
		switch ((id >> 16) & 0xffff) {
		case SH7763_DEVICE_ID:
		case SH7780_DEVICE_ID:
		case SH7781_DEVICE_ID:
		case SH7785_DEVICE_ID:
			match = 1;
			break;
		}
	}

	if (unlikely(!match)) {
		printk(KERN_ERR "PCI: This is not an SH7780 (%x)\n", id);
		return -ENODEV;
	}

	if ((ret = sh4_pci_check_direct(chan)) != 0)
		return ret;

	return pcibios_init_platform();
}

int __init sh7780_pcic_init(struct pci_channel *chan,
			    struct sh4_pci_address_map *map)
{
	u32 word;

	pci_write_reg(chan, PCI_CLASS_BRIDGE_HOST >> 8, SH7780_PCIBCC);
	pci_write_reg(chan, PCI_CLASS_BRIDGE_HOST & 0xff, SH7780_PCISUB);

	/* set the command/status bits to:
	 * Wait Cycle Control + Parity Enable + Bus Master +
	 * Mem space enable
	 */
	pci_write_reg(chan, 0x00000046, SH7780_PCICMD);

	/* Set IO and Mem windows to local address
	 * Make PCI and local address the same for easy 1 to 1 mapping
	 */
	pci_write_reg(chan, map->window0.size - 0xfffff, SH4_PCILSR0);
	pci_write_reg(chan, map->window1.size - 0xfffff, SH4_PCILSR1);
	/* Set the values on window 0 PCI config registers */
	pci_write_reg(chan, map->window0.base, SH4_PCILAR0);
	pci_write_reg(chan, map->window0.base, SH7780_PCIMBAR0);
	/* Set the values on window 1 PCI config registers */
	pci_write_reg(chan, map->window1.base, SH4_PCILAR1);
	pci_write_reg(chan, map->window1.base, SH7780_PCIMBAR1);

	/* Apply any last-minute PCIC fixups */
	pci_fixup_pcic(chan);

	/* SH7780 init done, set central function init complete */
	/* use round robin mode to stop a device starving/overruning */
	word = SH4_PCICR_PREFIX | SH4_PCICR_CFIN | SH4_PCICR_FTO;
	pci_write_reg(chan, word, SH4_PCICR);

	return 0;
}
