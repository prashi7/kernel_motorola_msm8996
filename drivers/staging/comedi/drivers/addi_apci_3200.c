#include <linux/module.h>
#include <linux/pci.h>

#include <asm/i387.h>

#include "../comedidev.h"
#include "comedi_fc.h"
#include "amcc_s5933.h"

#include "addi-data/addi_common.h"

static void fpu_begin(void)
{
	kernel_fpu_begin();
}

static void fpu_end(void)
{
	kernel_fpu_end();
}

#include "addi-data/addi_eeprom.c"
#include "addi-data/hwdrv_apci3200.c"
#include "addi-data/addi_common.c"

enum apci3200_boardid {
	BOARD_APCI3200,
	BOARD_APCI3300,
};

static const struct addi_board apci3200_boardtypes[] = {
	[BOARD_APCI3200] = {
		.pc_DriverName		= "apci3200",
		.i_IorangeBase1		= 256,
		.i_PCIEeprom		= 1,
		.pc_EepromChip		= "S5920",
		.i_NbrAiChannel		= 16,
		.i_NbrAiChannelDiff	= 8,
		.i_AiChannelList	= 16,
		.i_AiMaxdata		= 0x3ffff,
		.pr_AiRangelist		= &range_apci3200_ai,
		.i_NbrDiChannel		= 4,
		.i_NbrDoChannel		= 4,
		.ui_MinAcquisitiontimeNs = 10000,
		.ui_MinDelaytimeNs	= 100000,
		.interrupt		= apci3200_interrupt,
		.reset			= apci3200_reset,
		.ai_config		= apci3200_ai_config,
		.ai_read		= apci3200_ai_read,
		.ai_write		= apci3200_ai_write,
		.ai_bits		= apci3200_ai_bits_test,
		.ai_cmdtest		= apci3200_ai_cmdtest,
		.ai_cmd			= apci3200_ai_cmd,
		.ai_cancel		= apci3200_cancel,
		.di_bits		= apci3200_di_insn_bits,
		.do_bits		= apci3200_do_insn_bits,
	},
	[BOARD_APCI3300] = {
		.pc_DriverName		= "apci3300",
		.i_IorangeBase1		= 256,
		.i_PCIEeprom		= 1,
		.pc_EepromChip		= "S5920",
		.i_NbrAiChannelDiff	= 8,
		.i_AiChannelList	= 8,
		.i_AiMaxdata		= 0x3ffff,
		.pr_AiRangelist		= &range_apci3300_ai,
		.i_NbrDiChannel		= 4,
		.i_NbrDoChannel		= 4,
		.ui_MinAcquisitiontimeNs = 10000,
		.ui_MinDelaytimeNs	= 100000,
		.interrupt		= apci3200_interrupt,
		.reset			= apci3200_reset,
		.ai_config		= apci3200_ai_config,
		.ai_read		= apci3200_ai_read,
		.ai_write		= apci3200_ai_write,
		.ai_bits		= apci3200_ai_bits_test,
		.ai_cmdtest		= apci3200_ai_cmdtest,
		.ai_cmd			= apci3200_ai_cmd,
		.ai_cancel		= apci3200_cancel,
		.di_bits		= apci3200_di_insn_bits,
		.do_bits		= apci3200_do_insn_bits,
	},
};

static int apci3200_auto_attach(struct comedi_device *dev,
				unsigned long context)
{
	const struct addi_board *board = NULL;

	if (context < ARRAY_SIZE(apci3200_boardtypes))
		board = &apci3200_boardtypes[context];
	if (!board)
		return -ENODEV;
	dev->board_ptr = board;

	return addi_auto_attach(dev, context);
}

static struct comedi_driver apci3200_driver = {
	.driver_name	= "addi_apci_3200",
	.module		= THIS_MODULE,
	.auto_attach	= apci3200_auto_attach,
	.detach		= i_ADDI_Detach,
};

static int apci3200_pci_probe(struct pci_dev *dev,
			      const struct pci_device_id *id)
{
	return comedi_pci_auto_config(dev, &apci3200_driver, id->driver_data);
}

static const struct pci_device_id apci3200_pci_table[] = {
	{ PCI_VDEVICE(ADDIDATA, 0x3000), BOARD_APCI3200 },
	{ PCI_VDEVICE(ADDIDATA, 0x3007), BOARD_APCI3300 },
	{ 0 }
};
MODULE_DEVICE_TABLE(pci, apci3200_pci_table);

static struct pci_driver apci3200_pci_driver = {
	.name		= "addi_apci_3200",
	.id_table	= apci3200_pci_table,
	.probe		= apci3200_pci_probe,
	.remove		= comedi_pci_auto_unconfig,
};
module_comedi_pci_driver(apci3200_driver, apci3200_pci_driver);
