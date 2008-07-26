/*
 * Copyright (C) 2003 - 2006 NetXen, Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA  02111-1307, USA.
 *
 * The full GNU General Public License is included in this distribution
 * in the file called LICENSE.
 *
 * Contact Information:
 *    info@netxen.com
 * NetXen,
 * 3965 Freedom Circle, Fourth floor,
 * Santa Clara, CA 95054
 */

#ifndef __NETXEN_NIC_HDR_H_
#define __NETXEN_NIC_HDR_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>

#include <linux/spinlock.h>
#include <asm/irq.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <asm/uaccess.h>
#include <asm/string.h>		/* for memset */

/*
 * The basic unit of access when reading/writing control registers.
 */

typedef __le32 netxen_crbword_t;	/* single word in CRB space */

enum {
	NETXEN_HW_H0_CH_HUB_ADR = 0x05,
	NETXEN_HW_H1_CH_HUB_ADR = 0x0E,
	NETXEN_HW_H2_CH_HUB_ADR = 0x03,
	NETXEN_HW_H3_CH_HUB_ADR = 0x01,
	NETXEN_HW_H4_CH_HUB_ADR = 0x06,
	NETXEN_HW_H5_CH_HUB_ADR = 0x07,
	NETXEN_HW_H6_CH_HUB_ADR = 0x08
};

/*  Hub 0 */
enum {
	NETXEN_HW_MN_CRB_AGT_ADR = 0x15,
	NETXEN_HW_MS_CRB_AGT_ADR = 0x25
};

/*  Hub 1 */
enum {
	NETXEN_HW_PS_CRB_AGT_ADR = 0x73,
	NETXEN_HW_SS_CRB_AGT_ADR = 0x20,
	NETXEN_HW_RPMX3_CRB_AGT_ADR = 0x0b,
	NETXEN_HW_QMS_CRB_AGT_ADR = 0x00,
	NETXEN_HW_SQGS0_CRB_AGT_ADR = 0x01,
	NETXEN_HW_SQGS1_CRB_AGT_ADR = 0x02,
	NETXEN_HW_SQGS2_CRB_AGT_ADR = 0x03,
	NETXEN_HW_SQGS3_CRB_AGT_ADR = 0x04,
	NETXEN_HW_C2C0_CRB_AGT_ADR = 0x58,
	NETXEN_HW_C2C1_CRB_AGT_ADR = 0x59,
	NETXEN_HW_C2C2_CRB_AGT_ADR = 0x5a,
	NETXEN_HW_RPMX2_CRB_AGT_ADR = 0x0a,
	NETXEN_HW_RPMX4_CRB_AGT_ADR = 0x0c,
	NETXEN_HW_RPMX7_CRB_AGT_ADR = 0x0f,
	NETXEN_HW_RPMX9_CRB_AGT_ADR = 0x12,
	NETXEN_HW_SMB_CRB_AGT_ADR = 0x18
};

/*  Hub 2 */
enum {
	NETXEN_HW_NIU_CRB_AGT_ADR = 0x31,
	NETXEN_HW_I2C0_CRB_AGT_ADR = 0x19,
	NETXEN_HW_I2C1_CRB_AGT_ADR = 0x29,

	NETXEN_HW_SN_CRB_AGT_ADR = 0x10,
	NETXEN_HW_I2Q_CRB_AGT_ADR = 0x20,
	NETXEN_HW_LPC_CRB_AGT_ADR = 0x22,
	NETXEN_HW_ROMUSB_CRB_AGT_ADR = 0x21,
	NETXEN_HW_QM_CRB_AGT_ADR = 0x66,
	NETXEN_HW_SQG0_CRB_AGT_ADR = 0x60,
	NETXEN_HW_SQG1_CRB_AGT_ADR = 0x61,
	NETXEN_HW_SQG2_CRB_AGT_ADR = 0x62,
	NETXEN_HW_SQG3_CRB_AGT_ADR = 0x63,
	NETXEN_HW_RPMX1_CRB_AGT_ADR = 0x09,
	NETXEN_HW_RPMX5_CRB_AGT_ADR = 0x0d,
	NETXEN_HW_RPMX6_CRB_AGT_ADR = 0x0e,
	NETXEN_HW_RPMX8_CRB_AGT_ADR = 0x11
};

/*  Hub 3 */
enum {
	NETXEN_HW_PH_CRB_AGT_ADR = 0x1A,
	NETXEN_HW_SRE_CRB_AGT_ADR = 0x50,
	NETXEN_HW_EG_CRB_AGT_ADR = 0x51,
	NETXEN_HW_RPMX0_CRB_AGT_ADR = 0x08
};

/*  Hub 4 */
enum {
	NETXEN_HW_PEGN0_CRB_AGT_ADR = 0x40,
	NETXEN_HW_PEGN1_CRB_AGT_ADR,
	NETXEN_HW_PEGN2_CRB_AGT_ADR,
	NETXEN_HW_PEGN3_CRB_AGT_ADR,
	NETXEN_HW_PEGNI_CRB_AGT_ADR,
	NETXEN_HW_PEGND_CRB_AGT_ADR,
	NETXEN_HW_PEGNC_CRB_AGT_ADR,
	NETXEN_HW_PEGR0_CRB_AGT_ADR,
	NETXEN_HW_PEGR1_CRB_AGT_ADR,
	NETXEN_HW_PEGR2_CRB_AGT_ADR,
	NETXEN_HW_PEGR3_CRB_AGT_ADR,
	NETXEN_HW_PEGN4_CRB_AGT_ADR
};

/*  Hub 5 */
enum {
	NETXEN_HW_PEGS0_CRB_AGT_ADR = 0x40,
	NETXEN_HW_PEGS1_CRB_AGT_ADR,
	NETXEN_HW_PEGS2_CRB_AGT_ADR,
	NETXEN_HW_PEGS3_CRB_AGT_ADR,
	NETXEN_HW_PEGSI_CRB_AGT_ADR,
	NETXEN_HW_PEGSD_CRB_AGT_ADR,
	NETXEN_HW_PEGSC_CRB_AGT_ADR
};

/*  Hub 6 */
enum {
	NETXEN_HW_CAS0_CRB_AGT_ADR = 0x46,
	NETXEN_HW_CAS1_CRB_AGT_ADR = 0x47,
	NETXEN_HW_CAS2_CRB_AGT_ADR = 0x48,
	NETXEN_HW_CAS3_CRB_AGT_ADR = 0x49,
	NETXEN_HW_NCM_CRB_AGT_ADR = 0x16,
	NETXEN_HW_TMR_CRB_AGT_ADR = 0x17,
	NETXEN_HW_XDMA_CRB_AGT_ADR = 0x05,
	NETXEN_HW_OCM0_CRB_AGT_ADR = 0x06,
	NETXEN_HW_OCM1_CRB_AGT_ADR = 0x07
};

/*  Floaters - non existent modules */
#define NETXEN_HW_EFC_RPMX0_CRB_AGT_ADR	0x67

/*  This field defines PCI/X adr [25:20] of agents on the CRB */
enum {
	NETXEN_HW_PX_MAP_CRB_PH = 0,
	NETXEN_HW_PX_MAP_CRB_PS,
	NETXEN_HW_PX_MAP_CRB_MN,
	NETXEN_HW_PX_MAP_CRB_MS,
	NETXEN_HW_PX_MAP_CRB_PGR1,
	NETXEN_HW_PX_MAP_CRB_SRE,
	NETXEN_HW_PX_MAP_CRB_NIU,
	NETXEN_HW_PX_MAP_CRB_QMN,
	NETXEN_HW_PX_MAP_CRB_SQN0,
	NETXEN_HW_PX_MAP_CRB_SQN1,
	NETXEN_HW_PX_MAP_CRB_SQN2,
	NETXEN_HW_PX_MAP_CRB_SQN3,
	NETXEN_HW_PX_MAP_CRB_QMS,
	NETXEN_HW_PX_MAP_CRB_SQS0,
	NETXEN_HW_PX_MAP_CRB_SQS1,
	NETXEN_HW_PX_MAP_CRB_SQS2,
	NETXEN_HW_PX_MAP_CRB_SQS3,
	NETXEN_HW_PX_MAP_CRB_PGN0,
	NETXEN_HW_PX_MAP_CRB_PGN1,
	NETXEN_HW_PX_MAP_CRB_PGN2,
	NETXEN_HW_PX_MAP_CRB_PGN3,
	NETXEN_HW_PX_MAP_CRB_PGND,
	NETXEN_HW_PX_MAP_CRB_PGNI,
	NETXEN_HW_PX_MAP_CRB_PGS0,
	NETXEN_HW_PX_MAP_CRB_PGS1,
	NETXEN_HW_PX_MAP_CRB_PGS2,
	NETXEN_HW_PX_MAP_CRB_PGS3,
	NETXEN_HW_PX_MAP_CRB_PGSD,
	NETXEN_HW_PX_MAP_CRB_PGSI,
	NETXEN_HW_PX_MAP_CRB_SN,
	NETXEN_HW_PX_MAP_CRB_PGR2,
	NETXEN_HW_PX_MAP_CRB_EG,
	NETXEN_HW_PX_MAP_CRB_PH2,
	NETXEN_HW_PX_MAP_CRB_PS2,
	NETXEN_HW_PX_MAP_CRB_CAM,
	NETXEN_HW_PX_MAP_CRB_CAS0,
	NETXEN_HW_PX_MAP_CRB_CAS1,
	NETXEN_HW_PX_MAP_CRB_CAS2,
	NETXEN_HW_PX_MAP_CRB_C2C0,
	NETXEN_HW_PX_MAP_CRB_C2C1,
	NETXEN_HW_PX_MAP_CRB_TIMR,
	NETXEN_HW_PX_MAP_CRB_PGR3,
	NETXEN_HW_PX_MAP_CRB_RPMX1,
	NETXEN_HW_PX_MAP_CRB_RPMX2,
	NETXEN_HW_PX_MAP_CRB_RPMX3,
	NETXEN_HW_PX_MAP_CRB_RPMX4,
	NETXEN_HW_PX_MAP_CRB_RPMX5,
	NETXEN_HW_PX_MAP_CRB_RPMX6,
	NETXEN_HW_PX_MAP_CRB_RPMX7,
	NETXEN_HW_PX_MAP_CRB_XDMA,
	NETXEN_HW_PX_MAP_CRB_I2Q,
	NETXEN_HW_PX_MAP_CRB_ROMUSB,
	NETXEN_HW_PX_MAP_CRB_CAS3,
	NETXEN_HW_PX_MAP_CRB_RPMX0,
	NETXEN_HW_PX_MAP_CRB_RPMX8,
	NETXEN_HW_PX_MAP_CRB_RPMX9,
	NETXEN_HW_PX_MAP_CRB_OCM0,
	NETXEN_HW_PX_MAP_CRB_OCM1,
	NETXEN_HW_PX_MAP_CRB_SMB,
	NETXEN_HW_PX_MAP_CRB_I2C0,
	NETXEN_HW_PX_MAP_CRB_I2C1,
	NETXEN_HW_PX_MAP_CRB_LPC,
	NETXEN_HW_PX_MAP_CRB_PGNC,
	NETXEN_HW_PX_MAP_CRB_PGR0
};

/*  This field defines CRB adr [31:20] of the agents */

#define NETXEN_HW_CRB_HUB_AGT_ADR_MN	\
	((NETXEN_HW_H0_CH_HUB_ADR << 7) | NETXEN_HW_MN_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_PH	\
	((NETXEN_HW_H0_CH_HUB_ADR << 7) | NETXEN_HW_PH_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_MS	\
	((NETXEN_HW_H0_CH_HUB_ADR << 7) | NETXEN_HW_MS_CRB_AGT_ADR)

#define NETXEN_HW_CRB_HUB_AGT_ADR_PS	\
	((NETXEN_HW_H1_CH_HUB_ADR << 7) | NETXEN_HW_PS_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_SS	\
	((NETXEN_HW_H1_CH_HUB_ADR << 7) | NETXEN_HW_SS_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_RPMX3	\
	((NETXEN_HW_H1_CH_HUB_ADR << 7) | NETXEN_HW_RPMX3_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_QMS	\
	((NETXEN_HW_H1_CH_HUB_ADR << 7) | NETXEN_HW_QMS_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_SQS0	\
	((NETXEN_HW_H1_CH_HUB_ADR << 7) | NETXEN_HW_SQGS0_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_SQS1	\
	((NETXEN_HW_H1_CH_HUB_ADR << 7) | NETXEN_HW_SQGS1_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_SQS2	\
	((NETXEN_HW_H1_CH_HUB_ADR << 7) | NETXEN_HW_SQGS2_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_SQS3	\
	((NETXEN_HW_H1_CH_HUB_ADR << 7) | NETXEN_HW_SQGS3_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_C2C0	\
	((NETXEN_HW_H1_CH_HUB_ADR << 7) | NETXEN_HW_C2C0_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_C2C1	\
	((NETXEN_HW_H1_CH_HUB_ADR << 7) | NETXEN_HW_C2C1_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_RPMX2	\
	((NETXEN_HW_H1_CH_HUB_ADR << 7) | NETXEN_HW_RPMX2_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_RPMX4	\
	((NETXEN_HW_H1_CH_HUB_ADR << 7) | NETXEN_HW_RPMX4_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_RPMX7	\
	((NETXEN_HW_H1_CH_HUB_ADR << 7) | NETXEN_HW_RPMX7_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_RPMX9	\
	((NETXEN_HW_H1_CH_HUB_ADR << 7) | NETXEN_HW_RPMX9_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_SMB	\
	((NETXEN_HW_H1_CH_HUB_ADR << 7) | NETXEN_HW_SMB_CRB_AGT_ADR)

#define NETXEN_HW_CRB_HUB_AGT_ADR_NIU	\
	((NETXEN_HW_H2_CH_HUB_ADR << 7) | NETXEN_HW_NIU_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_I2C0	\
	((NETXEN_HW_H2_CH_HUB_ADR << 7) | NETXEN_HW_I2C0_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_I2C1	\
	((NETXEN_HW_H2_CH_HUB_ADR << 7) | NETXEN_HW_I2C1_CRB_AGT_ADR)

#define NETXEN_HW_CRB_HUB_AGT_ADR_SRE	\
	((NETXEN_HW_H3_CH_HUB_ADR << 7) | NETXEN_HW_SRE_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_EG	\
	((NETXEN_HW_H3_CH_HUB_ADR << 7) | NETXEN_HW_EG_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_RPMX0	\
	((NETXEN_HW_H3_CH_HUB_ADR << 7) | NETXEN_HW_RPMX0_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_QMN	\
	((NETXEN_HW_H3_CH_HUB_ADR << 7) | NETXEN_HW_QM_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_SQN0	\
	((NETXEN_HW_H3_CH_HUB_ADR << 7) | NETXEN_HW_SQG0_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_SQN1	\
	((NETXEN_HW_H3_CH_HUB_ADR << 7) | NETXEN_HW_SQG1_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_SQN2	\
	((NETXEN_HW_H3_CH_HUB_ADR << 7) | NETXEN_HW_SQG2_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_SQN3	\
	((NETXEN_HW_H3_CH_HUB_ADR << 7) | NETXEN_HW_SQG3_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_RPMX1	\
	((NETXEN_HW_H3_CH_HUB_ADR << 7) | NETXEN_HW_RPMX1_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_RPMX5	\
	((NETXEN_HW_H3_CH_HUB_ADR << 7) | NETXEN_HW_RPMX5_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_RPMX6	\
	((NETXEN_HW_H3_CH_HUB_ADR << 7) | NETXEN_HW_RPMX6_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_RPMX8	\
	((NETXEN_HW_H3_CH_HUB_ADR << 7) | NETXEN_HW_RPMX8_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_CAS0	\
	((NETXEN_HW_H3_CH_HUB_ADR << 7) | NETXEN_HW_CAS0_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_CAS1	\
	((NETXEN_HW_H3_CH_HUB_ADR << 7) | NETXEN_HW_CAS1_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_CAS2	\
	((NETXEN_HW_H3_CH_HUB_ADR << 7) | NETXEN_HW_CAS2_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_CAS3	\
	((NETXEN_HW_H3_CH_HUB_ADR << 7) | NETXEN_HW_CAS3_CRB_AGT_ADR)

#define NETXEN_HW_CRB_HUB_AGT_ADR_PGNI	\
	((NETXEN_HW_H4_CH_HUB_ADR << 7) | NETXEN_HW_PEGNI_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_PGND	\
	((NETXEN_HW_H4_CH_HUB_ADR << 7) | NETXEN_HW_PEGND_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_PGN0	\
	((NETXEN_HW_H4_CH_HUB_ADR << 7) | NETXEN_HW_PEGN0_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_PGN1	\
	((NETXEN_HW_H4_CH_HUB_ADR << 7) | NETXEN_HW_PEGN1_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_PGN2	\
	((NETXEN_HW_H4_CH_HUB_ADR << 7) | NETXEN_HW_PEGN2_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_PGN3	\
	((NETXEN_HW_H4_CH_HUB_ADR << 7) | NETXEN_HW_PEGN3_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_PGN4	\
	((NETXEN_HW_H4_CH_HUB_ADR << 7) | NETXEN_HW_PEGN4_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_PGNC	\
	((NETXEN_HW_H4_CH_HUB_ADR << 7) | NETXEN_HW_PEGNC_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_PGR0	\
	((NETXEN_HW_H4_CH_HUB_ADR << 7) | NETXEN_HW_PEGR0_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_PGR1	\
	((NETXEN_HW_H4_CH_HUB_ADR << 7) | NETXEN_HW_PEGR1_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_PGR2	\
	((NETXEN_HW_H4_CH_HUB_ADR << 7) | NETXEN_HW_PEGR2_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_PGR3	\
	((NETXEN_HW_H4_CH_HUB_ADR << 7) | NETXEN_HW_PEGR3_CRB_AGT_ADR)

#define NETXEN_HW_CRB_HUB_AGT_ADR_PGSI	\
	((NETXEN_HW_H5_CH_HUB_ADR << 7) | NETXEN_HW_PEGSI_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_PGSD	\
	((NETXEN_HW_H5_CH_HUB_ADR << 7) | NETXEN_HW_PEGSD_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_PGS0	\
	((NETXEN_HW_H5_CH_HUB_ADR << 7) | NETXEN_HW_PEGS0_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_PGS1	\
	((NETXEN_HW_H5_CH_HUB_ADR << 7) | NETXEN_HW_PEGS1_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_PGS2	\
	((NETXEN_HW_H5_CH_HUB_ADR << 7) | NETXEN_HW_PEGS2_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_PGS3	\
	((NETXEN_HW_H5_CH_HUB_ADR << 7) | NETXEN_HW_PEGS3_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_PGSC	\
	((NETXEN_HW_H5_CH_HUB_ADR << 7) | NETXEN_HW_PEGSC_CRB_AGT_ADR)

#define NETXEN_HW_CRB_HUB_AGT_ADR_CAM	\
	((NETXEN_HW_H6_CH_HUB_ADR << 7) | NETXEN_HW_NCM_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_TIMR	\
	((NETXEN_HW_H6_CH_HUB_ADR << 7) | NETXEN_HW_TMR_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_XDMA	\
	((NETXEN_HW_H6_CH_HUB_ADR << 7) | NETXEN_HW_XDMA_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_SN	\
	((NETXEN_HW_H6_CH_HUB_ADR << 7) | NETXEN_HW_SN_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_I2Q	\
	((NETXEN_HW_H6_CH_HUB_ADR << 7) | NETXEN_HW_I2Q_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_ROMUSB	\
	((NETXEN_HW_H6_CH_HUB_ADR << 7) | NETXEN_HW_ROMUSB_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_OCM0	\
	((NETXEN_HW_H6_CH_HUB_ADR << 7) | NETXEN_HW_OCM0_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_OCM1	\
	((NETXEN_HW_H6_CH_HUB_ADR << 7) | NETXEN_HW_OCM1_CRB_AGT_ADR)
#define NETXEN_HW_CRB_HUB_AGT_ADR_LPC	\
	((NETXEN_HW_H6_CH_HUB_ADR << 7) | NETXEN_HW_LPC_CRB_AGT_ADR)

/*
 * MAX_RCV_CTX : The number of receive contexts that are available on
 * the phantom.
 */
#define MAX_RCV_CTX			1

#define NETXEN_SRE_INT_STATUS		(NETXEN_CRB_SRE + 0x00034)
#define NETXEN_SRE_PBI_ACTIVE_STATUS	(NETXEN_CRB_SRE + 0x01014)
#define NETXEN_SRE_L1RE_CTL		(NETXEN_CRB_SRE + 0x03000)
#define NETXEN_SRE_L2RE_CTL		(NETXEN_CRB_SRE + 0x05000)
#define NETXEN_SRE_BUF_CTL		(NETXEN_CRB_SRE + 0x01000)

#define	NETXEN_DMA_BASE(U)	(NETXEN_CRB_PCIX_MD + 0x20000 + ((U)<<16))
#define	NETXEN_DMA_COMMAND(U)	(NETXEN_DMA_BASE(U) + 0x00008)

#define NETXEN_I2Q_CLR_PCI_HI	(NETXEN_CRB_I2Q + 0x00034)

#define PEG_NETWORK_BASE(N)	(NETXEN_CRB_PEG_NET_0 + (((N)&3) << 20))
#define CRB_REG_EX_PC		0x3c

#define ROMUSB_GLB	(NETXEN_CRB_ROMUSB + 0x00000)
#define ROMUSB_ROM	(NETXEN_CRB_ROMUSB + 0x10000)

#define NETXEN_ROMUSB_GLB_STATUS	(ROMUSB_GLB + 0x0004)
#define NETXEN_ROMUSB_GLB_SW_RESET	(ROMUSB_GLB + 0x0008)
#define NETXEN_ROMUSB_GLB_PAD_GPIO_I	(ROMUSB_GLB + 0x000c)
#define NETXEN_ROMUSB_GLB_CAS_RST	(ROMUSB_GLB + 0x0038)
#define NETXEN_ROMUSB_GLB_TEST_MUX_SEL	(ROMUSB_GLB + 0x0044)
#define NETXEN_ROMUSB_GLB_PEGTUNE_DONE	(ROMUSB_GLB + 0x005c)
#define NETXEN_ROMUSB_GLB_CHIP_CLK_CTRL	(ROMUSB_GLB + 0x00A8)

#define NETXEN_ROMUSB_GPIO(n)		(ROMUSB_GLB + 0x60 + (4 * (n)))

#define NETXEN_ROMUSB_ROM_INSTR_OPCODE	(ROMUSB_ROM + 0x0004)
#define NETXEN_ROMUSB_ROM_ADDRESS	(ROMUSB_ROM + 0x0008)
#define NETXEN_ROMUSB_ROM_WDATA		(ROMUSB_ROM + 0x000c)
#define NETXEN_ROMUSB_ROM_ABYTE_CNT	(ROMUSB_ROM + 0x0010)
#define NETXEN_ROMUSB_ROM_DUMMY_BYTE_CNT (ROMUSB_ROM + 0x0014)
#define NETXEN_ROMUSB_ROM_RDATA		(ROMUSB_ROM + 0x0018)

/* Lock IDs for ROM lock */
#define ROM_LOCK_DRIVER	0x0d417340

/******************************************************************************
*
*    Definitions specific to M25P flash
*
*******************************************************************************
*   Instructions
*/
#define M25P_INSTR_WREN		0x06
#define M25P_INSTR_WRDI		0x04
#define M25P_INSTR_RDID		0x9f
#define M25P_INSTR_RDSR		0x05
#define M25P_INSTR_WRSR		0x01
#define M25P_INSTR_READ		0x03
#define M25P_INSTR_FAST_READ	0x0b
#define M25P_INSTR_PP		0x02
#define M25P_INSTR_SE		0xd8
#define M25P_INSTR_BE		0xc7
#define M25P_INSTR_DP		0xb9
#define M25P_INSTR_RES		0xab

/* all are 1MB windows */

#define NETXEN_PCI_CRB_WINDOWSIZE	0x00100000
#define NETXEN_PCI_CRB_WINDOW(A)	\
	(NETXEN_PCI_CRBSPACE + (A)*NETXEN_PCI_CRB_WINDOWSIZE)

#define NETXEN_CRB_NIU		NETXEN_PCI_CRB_WINDOW(NETXEN_HW_PX_MAP_CRB_NIU)
#define NETXEN_CRB_SRE		NETXEN_PCI_CRB_WINDOW(NETXEN_HW_PX_MAP_CRB_SRE)
#define NETXEN_CRB_ROMUSB	\
	NETXEN_PCI_CRB_WINDOW(NETXEN_HW_PX_MAP_CRB_ROMUSB)
#define NETXEN_CRB_I2Q		NETXEN_PCI_CRB_WINDOW(NETXEN_HW_PX_MAP_CRB_I2Q)
#define NETXEN_CRB_SMB		NETXEN_PCI_CRB_WINDOW(NETXEN_HW_PX_MAP_CRB_SMB)
#define NETXEN_CRB_MAX		NETXEN_PCI_CRB_WINDOW(64)

#define NETXEN_CRB_PCIX_HOST	NETXEN_PCI_CRB_WINDOW(NETXEN_HW_PX_MAP_CRB_PH)
#define NETXEN_CRB_PCIX_HOST2	NETXEN_PCI_CRB_WINDOW(NETXEN_HW_PX_MAP_CRB_PH2)
#define NETXEN_CRB_PEG_NET_0	NETXEN_PCI_CRB_WINDOW(NETXEN_HW_PX_MAP_CRB_PGN0)
#define NETXEN_CRB_PEG_NET_1	NETXEN_PCI_CRB_WINDOW(NETXEN_HW_PX_MAP_CRB_PGN1)
#define NETXEN_CRB_PEG_NET_2	NETXEN_PCI_CRB_WINDOW(NETXEN_HW_PX_MAP_CRB_PGN2)
#define NETXEN_CRB_PEG_NET_3	NETXEN_PCI_CRB_WINDOW(NETXEN_HW_PX_MAP_CRB_PGN3)
#define NETXEN_CRB_PEG_NET_D	NETXEN_PCI_CRB_WINDOW(NETXEN_HW_PX_MAP_CRB_PGND)
#define NETXEN_CRB_PEG_NET_I	NETXEN_PCI_CRB_WINDOW(NETXEN_HW_PX_MAP_CRB_PGNI)
#define NETXEN_CRB_DDR_NET	NETXEN_PCI_CRB_WINDOW(NETXEN_HW_PX_MAP_CRB_MN)
#define NETXEN_CRB_QDR_NET	NETXEN_PCI_CRB_WINDOW(NETXEN_HW_PX_MAP_CRB_SN)

#define NETXEN_CRB_PCIX_MD	NETXEN_PCI_CRB_WINDOW(NETXEN_HW_PX_MAP_CRB_PS)
#define NETXEN_CRB_PCIE		NETXEN_CRB_PCIX_MD

#define ISR_INT_VECTOR		(NETXEN_PCIX_PS_REG(PCIX_INT_VECTOR))
#define ISR_INT_MASK		(NETXEN_PCIX_PS_REG(PCIX_INT_MASK))
#define ISR_INT_MASK_SLOW	(NETXEN_PCIX_PS_REG(PCIX_INT_MASK))
#define ISR_INT_TARGET_STATUS	(NETXEN_PCIX_PS_REG(PCIX_TARGET_STATUS))
#define ISR_INT_TARGET_MASK	(NETXEN_PCIX_PS_REG(PCIX_TARGET_MASK))
#define ISR_INT_TARGET_STATUS_F1   (NETXEN_PCIX_PS_REG(PCIX_TARGET_STATUS_F1))
#define ISR_INT_TARGET_MASK_F1     (NETXEN_PCIX_PS_REG(PCIX_TARGET_MASK_F1))
#define ISR_INT_TARGET_STATUS_F2   (NETXEN_PCIX_PS_REG(PCIX_TARGET_STATUS_F2))
#define ISR_INT_TARGET_MASK_F2     (NETXEN_PCIX_PS_REG(PCIX_TARGET_MASK_F2))
#define ISR_INT_TARGET_STATUS_F3   (NETXEN_PCIX_PS_REG(PCIX_TARGET_STATUS_F3))
#define ISR_INT_TARGET_MASK_F3     (NETXEN_PCIX_PS_REG(PCIX_TARGET_MASK_F3))
#define ISR_INT_TARGET_STATUS_F4   (NETXEN_PCIX_PS_REG(PCIX_TARGET_STATUS_F4))
#define ISR_INT_TARGET_MASK_F4     (NETXEN_PCIX_PS_REG(PCIX_TARGET_MASK_F4))
#define ISR_INT_TARGET_STATUS_F5   (NETXEN_PCIX_PS_REG(PCIX_TARGET_STATUS_F5))
#define ISR_INT_TARGET_MASK_F5     (NETXEN_PCIX_PS_REG(PCIX_TARGET_MASK_F5))
#define ISR_INT_TARGET_STATUS_F6   (NETXEN_PCIX_PS_REG(PCIX_TARGET_STATUS_F6))
#define ISR_INT_TARGET_MASK_F6     (NETXEN_PCIX_PS_REG(PCIX_TARGET_MASK_F6))
#define ISR_INT_TARGET_STATUS_F7   (NETXEN_PCIX_PS_REG(PCIX_TARGET_STATUS_F7))
#define ISR_INT_TARGET_MASK_F7     (NETXEN_PCIX_PS_REG(PCIX_TARGET_MASK_F7))

#define NETXEN_PCI_MAPSIZE	128
#define NETXEN_PCI_DDR_NET	(0x00000000UL)
#define NETXEN_PCI_QDR_NET	(0x04000000UL)
#define NETXEN_PCI_DIRECT_CRB	(0x04400000UL)
#define NETXEN_PCI_CAMQM	(0x04800000UL)
#define NETXEN_PCI_CAMQM_MAX	(0x04ffffffUL)
#define NETXEN_PCI_OCM0		(0x05000000UL)
#define NETXEN_PCI_OCM0_MAX	(0x050fffffUL)
#define NETXEN_PCI_OCM1		(0x05100000UL)
#define NETXEN_PCI_OCM1_MAX	(0x051fffffUL)
#define NETXEN_PCI_CRBSPACE	(0x06000000UL)
#define NETXEN_PCI_128MB_SIZE	(0x08000000UL)
#define NETXEN_PCI_32MB_SIZE	(0x02000000UL)
#define NETXEN_PCI_2MB_SIZE	(0x00200000UL)

#define NETXEN_PCI_MN_2M	(0)
#define NETXEN_PCI_MS_2M	(0x80000)
#define NETXEN_PCI_OCM0_2M	(0x000c0000UL)
#define NETXEN_PCI_CAMQM_2M_BASE	(0x000ff800UL)
#define NETXEN_PCI_CAMQM_2M_END		(0x04800800UL)

#define NETXEN_CRB_CAM	NETXEN_PCI_CRB_WINDOW(NETXEN_HW_PX_MAP_CRB_CAM)

#define NETXEN_ADDR_DDR_NET	(0x0000000000000000ULL)
#define NETXEN_ADDR_DDR_NET_MAX (0x000000000fffffffULL)
#define NETXEN_ADDR_OCM0	(0x0000000200000000ULL)
#define NETXEN_ADDR_OCM0_MAX	(0x00000002000fffffULL)
#define NETXEN_ADDR_OCM1	(0x0000000200400000ULL)
#define NETXEN_ADDR_OCM1_MAX	(0x00000002004fffffULL)
#define NETXEN_ADDR_QDR_NET	(0x0000000300000000ULL)
#define NETXEN_ADDR_QDR_NET_MAX_P2 (0x00000003003fffffULL)
#define NETXEN_ADDR_QDR_NET_MAX_P3 (0x0000000303ffffffULL)

/*
 *   Register offsets for MN
 */
#define	NETXEN_MIU_CONTROL	(0x000)
#define	NETXEN_MIU_MN_CONTROL	(NETXEN_CRB_DDR_NET+NETXEN_MIU_CONTROL)

	/* 200ms delay in each loop */
#define	NETXEN_NIU_PHY_WAITLEN		200000
	/* 10 seconds before we give up */
#define	NETXEN_NIU_PHY_WAITMAX		50
#define	NETXEN_NIU_MAX_GBE_PORTS	4
#define	NETXEN_NIU_MAX_XG_PORTS		2

#define	NETXEN_NIU_MODE			(NETXEN_CRB_NIU + 0x00000)

#define	NETXEN_NIU_XG_SINGLE_TERM	(NETXEN_CRB_NIU + 0x00004)
#define	NETXEN_NIU_XG_DRIVE_HI		(NETXEN_CRB_NIU + 0x00008)
#define	NETXEN_NIU_XG_DRIVE_LO		(NETXEN_CRB_NIU + 0x0000c)
#define	NETXEN_NIU_XG_DTX		(NETXEN_CRB_NIU + 0x00010)
#define	NETXEN_NIU_XG_DEQ		(NETXEN_CRB_NIU + 0x00014)
#define	NETXEN_NIU_XG_WORD_ALIGN	(NETXEN_CRB_NIU + 0x00018)
#define	NETXEN_NIU_XG_RESET		(NETXEN_CRB_NIU + 0x0001c)
#define	NETXEN_NIU_XG_POWER_DOWN	(NETXEN_CRB_NIU + 0x00020)
#define	NETXEN_NIU_XG_RESET_PLL		(NETXEN_CRB_NIU + 0x00024)
#define	NETXEN_NIU_XG_SERDES_LOOPBACK	(NETXEN_CRB_NIU + 0x00028)
#define	NETXEN_NIU_XG_DO_BYTE_ALIGN	(NETXEN_CRB_NIU + 0x0002c)
#define	NETXEN_NIU_XG_TX_ENABLE		(NETXEN_CRB_NIU + 0x00030)
#define	NETXEN_NIU_XG_RX_ENABLE		(NETXEN_CRB_NIU + 0x00034)
#define	NETXEN_NIU_XG_STATUS		(NETXEN_CRB_NIU + 0x00038)
#define	NETXEN_NIU_XG_PAUSE_THRESHOLD	(NETXEN_CRB_NIU + 0x0003c)
#define	NETXEN_NIU_INT_MASK		(NETXEN_CRB_NIU + 0x00040)
#define	NETXEN_NIU_ACTIVE_INT		(NETXEN_CRB_NIU + 0x00044)
#define	NETXEN_NIU_MASKABLE_INT		(NETXEN_CRB_NIU + 0x00048)

#define NETXEN_NIU_STRAP_VALUE_SAVE_HIGHER	(NETXEN_CRB_NIU + 0x0004c)

#define	NETXEN_NIU_GB_SERDES_RESET	(NETXEN_CRB_NIU + 0x00050)
#define	NETXEN_NIU_GB0_GMII_MODE	(NETXEN_CRB_NIU + 0x00054)
#define	NETXEN_NIU_GB0_MII_MODE		(NETXEN_CRB_NIU + 0x00058)
#define	NETXEN_NIU_GB1_GMII_MODE	(NETXEN_CRB_NIU + 0x0005c)
#define	NETXEN_NIU_GB1_MII_MODE		(NETXEN_CRB_NIU + 0x00060)
#define	NETXEN_NIU_GB2_GMII_MODE	(NETXEN_CRB_NIU + 0x00064)
#define	NETXEN_NIU_GB2_MII_MODE		(NETXEN_CRB_NIU + 0x00068)
#define	NETXEN_NIU_GB3_GMII_MODE	(NETXEN_CRB_NIU + 0x0006c)
#define	NETXEN_NIU_GB3_MII_MODE		(NETXEN_CRB_NIU + 0x00070)
#define	NETXEN_NIU_REMOTE_LOOPBACK	(NETXEN_CRB_NIU + 0x00074)
#define	NETXEN_NIU_GB0_HALF_DUPLEX	(NETXEN_CRB_NIU + 0x00078)
#define	NETXEN_NIU_GB1_HALF_DUPLEX	(NETXEN_CRB_NIU + 0x0007c)
#define	NETXEN_NIU_RESET_SYS_FIFOS	(NETXEN_CRB_NIU + 0x00088)
#define	NETXEN_NIU_GB_CRC_DROP		(NETXEN_CRB_NIU + 0x0008c)
#define	NETXEN_NIU_GB_DROP_WRONGADDR	(NETXEN_CRB_NIU + 0x00090)
#define	NETXEN_NIU_TEST_MUX_CTL		(NETXEN_CRB_NIU + 0x00094)
#define	NETXEN_NIU_XG_PAUSE_CTL		(NETXEN_CRB_NIU + 0x00098)
#define	NETXEN_NIU_XG_PAUSE_LEVEL	(NETXEN_CRB_NIU + 0x000dc)
#define	NETXEN_NIU_XG_SEL		(NETXEN_CRB_NIU + 0x00128)
#define NETXEN_NIU_GB_PAUSE_CTL		(NETXEN_CRB_NIU + 0x0030c)

#define NETXEN_NIU_FULL_LEVEL_XG	(NETXEN_CRB_NIU + 0x00450)

#define NETXEN_NIU_XG1_RESET	    	(NETXEN_CRB_NIU + 0x0011c)
#define NETXEN_NIU_XG1_POWER_DOWN	(NETXEN_CRB_NIU + 0x00120)
#define NETXEN_NIU_XG1_RESET_PLL	(NETXEN_CRB_NIU + 0x00124)

#define NETXEN_MAC_ADDR_CNTL_REG	(NETXEN_CRB_NIU + 0x1000)

#define	NETXEN_MULTICAST_ADDR_HI_0	(NETXEN_CRB_NIU + 0x1010)
#define NETXEN_MULTICAST_ADDR_HI_1	(NETXEN_CRB_NIU + 0x1014)
#define NETXEN_MULTICAST_ADDR_HI_2	(NETXEN_CRB_NIU + 0x1018)
#define NETXEN_MULTICAST_ADDR_HI_3	(NETXEN_CRB_NIU + 0x101c)

#define NETXEN_UNICAST_ADDR_BASE	(NETXEN_CRB_NIU + 0x1080)
#define	NETXEN_MULTICAST_ADDR_BASE	(NETXEN_CRB_NIU + 0x1100)

#define	NETXEN_NIU_GB_MAC_CONFIG_0(I)		\
	(NETXEN_CRB_NIU + 0x30000 + (I)*0x10000)
#define	NETXEN_NIU_GB_MAC_CONFIG_1(I)		\
	(NETXEN_CRB_NIU + 0x30004 + (I)*0x10000)
#define	NETXEN_NIU_GB_MAC_IPG_IFG(I)		\
	(NETXEN_CRB_NIU + 0x30008 + (I)*0x10000)
#define	NETXEN_NIU_GB_HALF_DUPLEX_CTRL(I)	\
	(NETXEN_CRB_NIU + 0x3000c + (I)*0x10000)
#define	NETXEN_NIU_GB_MAX_FRAME_SIZE(I)		\
	(NETXEN_CRB_NIU + 0x30010 + (I)*0x10000)
#define	NETXEN_NIU_GB_TEST_REG(I)		\
	(NETXEN_CRB_NIU + 0x3001c + (I)*0x10000)
#define	NETXEN_NIU_GB_MII_MGMT_CONFIG(I)	\
	(NETXEN_CRB_NIU + 0x30020 + (I)*0x10000)
#define	NETXEN_NIU_GB_MII_MGMT_COMMAND(I)	\
	(NETXEN_CRB_NIU + 0x30024 + (I)*0x10000)
#define	NETXEN_NIU_GB_MII_MGMT_ADDR(I)		\
	(NETXEN_CRB_NIU + 0x30028 + (I)*0x10000)
#define	NETXEN_NIU_GB_MII_MGMT_CTRL(I)		\
	(NETXEN_CRB_NIU + 0x3002c + (I)*0x10000)
#define	NETXEN_NIU_GB_MII_MGMT_STATUS(I)	\
	(NETXEN_CRB_NIU + 0x30030 + (I)*0x10000)
#define	NETXEN_NIU_GB_MII_MGMT_INDICATE(I)	\
	(NETXEN_CRB_NIU + 0x30034 + (I)*0x10000)
#define	NETXEN_NIU_GB_INTERFACE_CTRL(I)		\
	(NETXEN_CRB_NIU + 0x30038 + (I)*0x10000)
#define	NETXEN_NIU_GB_INTERFACE_STATUS(I)	\
	(NETXEN_CRB_NIU + 0x3003c + (I)*0x10000)
#define	NETXEN_NIU_GB_STATION_ADDR_0(I)		\
	(NETXEN_CRB_NIU + 0x30040 + (I)*0x10000)
#define	NETXEN_NIU_GB_STATION_ADDR_1(I)		\
	(NETXEN_CRB_NIU + 0x30044 + (I)*0x10000)

#define	NETXEN_NIU_XGE_CONFIG_0			(NETXEN_CRB_NIU + 0x70000)
#define	NETXEN_NIU_XGE_CONFIG_1			(NETXEN_CRB_NIU + 0x70004)
#define	NETXEN_NIU_XGE_IPG			(NETXEN_CRB_NIU + 0x70008)
#define	NETXEN_NIU_XGE_STATION_ADDR_0_HI	(NETXEN_CRB_NIU + 0x7000c)
#define	NETXEN_NIU_XGE_STATION_ADDR_0_1		(NETXEN_CRB_NIU + 0x70010)
#define	NETXEN_NIU_XGE_STATION_ADDR_1_LO	(NETXEN_CRB_NIU + 0x70014)
#define	NETXEN_NIU_XGE_STATUS			(NETXEN_CRB_NIU + 0x70018)
#define	NETXEN_NIU_XGE_MAX_FRAME_SIZE		(NETXEN_CRB_NIU + 0x7001c)
#define	NETXEN_NIU_XGE_PAUSE_FRAME_VALUE	(NETXEN_CRB_NIU + 0x70020)
#define	NETXEN_NIU_XGE_TX_BYTE_CNT		(NETXEN_CRB_NIU + 0x70024)
#define	NETXEN_NIU_XGE_TX_FRAME_CNT		(NETXEN_CRB_NIU + 0x70028)
#define	NETXEN_NIU_XGE_RX_BYTE_CNT		(NETXEN_CRB_NIU + 0x7002c)
#define	NETXEN_NIU_XGE_RX_FRAME_CNT		(NETXEN_CRB_NIU + 0x70030)
#define	NETXEN_NIU_XGE_AGGR_ERROR_CNT		(NETXEN_CRB_NIU + 0x70034)
#define	NETXEN_NIU_XGE_MULTICAST_FRAME_CNT 	(NETXEN_CRB_NIU + 0x70038)
#define	NETXEN_NIU_XGE_UNICAST_FRAME_CNT	(NETXEN_CRB_NIU + 0x7003c)
#define	NETXEN_NIU_XGE_CRC_ERROR_CNT		(NETXEN_CRB_NIU + 0x70040)
#define	NETXEN_NIU_XGE_OVERSIZE_FRAME_ERR	(NETXEN_CRB_NIU + 0x70044)
#define	NETXEN_NIU_XGE_UNDERSIZE_FRAME_ERR	(NETXEN_CRB_NIU + 0x70048)
#define	NETXEN_NIU_XGE_LOCAL_ERROR_CNT		(NETXEN_CRB_NIU + 0x7004c)
#define	NETXEN_NIU_XGE_REMOTE_ERROR_CNT		(NETXEN_CRB_NIU + 0x70050)
#define	NETXEN_NIU_XGE_CONTROL_CHAR_CNT		(NETXEN_CRB_NIU + 0x70054)
#define	NETXEN_NIU_XGE_PAUSE_FRAME_CNT		(NETXEN_CRB_NIU + 0x70058)
#define NETXEN_NIU_XG1_CONFIG_0			(NETXEN_CRB_NIU + 0x80000)
#define NETXEN_NIU_XG1_CONFIG_1			(NETXEN_CRB_NIU + 0x80004)
#define NETXEN_NIU_XG1_IPG			(NETXEN_CRB_NIU + 0x80008)
#define NETXEN_NIU_XG1_STATION_ADDR_0_HI	(NETXEN_CRB_NIU + 0x8000c)
#define NETXEN_NIU_XG1_STATION_ADDR_0_1		(NETXEN_CRB_NIU + 0x80010)
#define NETXEN_NIU_XG1_STATION_ADDR_1_LO	(NETXEN_CRB_NIU + 0x80014)
#define NETXEN_NIU_XG1_STATUS		    	(NETXEN_CRB_NIU + 0x80018)
#define NETXEN_NIU_XG1_MAX_FRAME_SIZE	   	(NETXEN_CRB_NIU + 0x8001c)
#define NETXEN_NIU_XG1_PAUSE_FRAME_VALUE	(NETXEN_CRB_NIU + 0x80020)
#define NETXEN_NIU_XG1_TX_BYTE_CNT		(NETXEN_CRB_NIU + 0x80024)
#define NETXEN_NIU_XG1_TX_FRAME_CNT	 	(NETXEN_CRB_NIU + 0x80028)
#define NETXEN_NIU_XG1_RX_BYTE_CNT	  	(NETXEN_CRB_NIU + 0x8002c)
#define NETXEN_NIU_XG1_RX_FRAME_CNT	 	(NETXEN_CRB_NIU + 0x80030)
#define NETXEN_NIU_XG1_AGGR_ERROR_CNT	   	(NETXEN_CRB_NIU + 0x80034)
#define NETXEN_NIU_XG1_MULTICAST_FRAME_CNT	(NETXEN_CRB_NIU + 0x80038)
#define NETXEN_NIU_XG1_UNICAST_FRAME_CNT	(NETXEN_CRB_NIU + 0x8003c)
#define NETXEN_NIU_XG1_CRC_ERROR_CNT		(NETXEN_CRB_NIU + 0x80040)
#define NETXEN_NIU_XG1_OVERSIZE_FRAME_ERR	(NETXEN_CRB_NIU + 0x80044)
#define NETXEN_NIU_XG1_UNDERSIZE_FRAME_ERR	(NETXEN_CRB_NIU + 0x80048)
#define NETXEN_NIU_XG1_LOCAL_ERROR_CNT		(NETXEN_CRB_NIU + 0x8004c)
#define NETXEN_NIU_XG1_REMOTE_ERROR_CNT		(NETXEN_CRB_NIU + 0x80050)
#define NETXEN_NIU_XG1_CONTROL_CHAR_CNT		(NETXEN_CRB_NIU + 0x80054)
#define NETXEN_NIU_XG1_PAUSE_FRAME_CNT		(NETXEN_CRB_NIU + 0x80058)

/* P3 802.3ap */
#define NETXEN_NIU_AP_MAC_CONFIG_0(I)      (NETXEN_CRB_NIU+0xa0000+(I)*0x10000)
#define NETXEN_NIU_AP_MAC_CONFIG_1(I)      (NETXEN_CRB_NIU+0xa0004+(I)*0x10000)
#define NETXEN_NIU_AP_MAC_IPG_IFG(I)       (NETXEN_CRB_NIU+0xa0008+(I)*0x10000)
#define NETXEN_NIU_AP_HALF_DUPLEX_CTRL(I)  (NETXEN_CRB_NIU+0xa000c+(I)*0x10000)
#define NETXEN_NIU_AP_MAX_FRAME_SIZE(I)    (NETXEN_CRB_NIU+0xa0010+(I)*0x10000)
#define NETXEN_NIU_AP_TEST_REG(I)          (NETXEN_CRB_NIU+0xa001c+(I)*0x10000)
#define NETXEN_NIU_AP_MII_MGMT_CONFIG(I)   (NETXEN_CRB_NIU+0xa0020+(I)*0x10000)
#define NETXEN_NIU_AP_MII_MGMT_COMMAND(I)  (NETXEN_CRB_NIU+0xa0024+(I)*0x10000)
#define NETXEN_NIU_AP_MII_MGMT_ADDR(I)     (NETXEN_CRB_NIU+0xa0028+(I)*0x10000)
#define NETXEN_NIU_AP_MII_MGMT_CTRL(I)     (NETXEN_CRB_NIU+0xa002c+(I)*0x10000)
#define NETXEN_NIU_AP_MII_MGMT_STATUS(I)   (NETXEN_CRB_NIU+0xa0030+(I)*0x10000)
#define NETXEN_NIU_AP_MII_MGMT_INDICATE(I) (NETXEN_CRB_NIU+0xa0034+(I)*0x10000)
#define NETXEN_NIU_AP_INTERFACE_CTRL(I)    (NETXEN_CRB_NIU+0xa0038+(I)*0x10000)
#define NETXEN_NIU_AP_INTERFACE_STATUS(I)  (NETXEN_CRB_NIU+0xa003c+(I)*0x10000)
#define NETXEN_NIU_AP_STATION_ADDR_0(I)    (NETXEN_CRB_NIU+0xa0040+(I)*0x10000)
#define NETXEN_NIU_AP_STATION_ADDR_1(I)    (NETXEN_CRB_NIU+0xa0044+(I)*0x10000)

/*
 *   Register offsets for MN
 */
#define	MIU_CONTROL	       (0x000)
#define MIU_TEST_AGT_CTRL      (0x090)
#define MIU_TEST_AGT_ADDR_LO   (0x094)
#define MIU_TEST_AGT_ADDR_HI   (0x098)
#define MIU_TEST_AGT_WRDATA_LO (0x0a0)
#define MIU_TEST_AGT_WRDATA_HI (0x0a4)
#define MIU_TEST_AGT_WRDATA(i) (0x0a0+(4*(i)))
#define MIU_TEST_AGT_RDDATA_LO (0x0a8)
#define MIU_TEST_AGT_RDDATA_HI (0x0ac)
#define MIU_TEST_AGT_RDDATA(i) (0x0a8+(4*(i)))
#define MIU_TEST_AGT_ADDR_MASK 0xfffffff8
#define MIU_TEST_AGT_UPPER_ADDR(off) (0)

/* MIU_TEST_AGT_CTRL flags. work for SIU as well */
#define MIU_TA_CTL_START        1
#define MIU_TA_CTL_ENABLE       2
#define MIU_TA_CTL_WRITE        4
#define MIU_TA_CTL_BUSY         8

#define SIU_TEST_AGT_CTRL      (0x060)
#define SIU_TEST_AGT_ADDR_LO   (0x064)
#define SIU_TEST_AGT_ADDR_HI   (0x078)
#define SIU_TEST_AGT_WRDATA_LO (0x068)
#define SIU_TEST_AGT_WRDATA_HI (0x06c)
#define SIU_TEST_AGT_WRDATA(i) (0x068+(4*(i)))
#define SIU_TEST_AGT_RDDATA_LO (0x070)
#define SIU_TEST_AGT_RDDATA_HI (0x074)
#define SIU_TEST_AGT_RDDATA(i) (0x070+(4*(i)))

#define SIU_TEST_AGT_ADDR_MASK 0x3ffff8
#define SIU_TEST_AGT_UPPER_ADDR(off) ((off)>>22)

/* XG Link status */
#define XG_LINK_UP	0x10
#define XG_LINK_DOWN	0x20

#define XG_LINK_UP_P3	0x01
#define XG_LINK_DOWN_P3	0x02
#define XG_LINK_STATE_P3_MASK 0xf
#define XG_LINK_STATE_P3(pcifn,val) \
	(((val) >> ((pcifn) * 4)) & XG_LINK_STATE_P3_MASK)

#define NETXEN_CAM_RAM_BASE	(NETXEN_CRB_CAM + 0x02000)
#define NETXEN_CAM_RAM(reg)	(NETXEN_CAM_RAM_BASE + (reg))
#define NETXEN_FW_VERSION_MAJOR (NETXEN_CAM_RAM(0x150))
#define NETXEN_FW_VERSION_MINOR (NETXEN_CAM_RAM(0x154))
#define NETXEN_FW_VERSION_SUB	(NETXEN_CAM_RAM(0x158))
#define NETXEN_ROM_LOCK_ID	(NETXEN_CAM_RAM(0x100))
#define NETXEN_CRB_WIN_LOCK_ID	(NETXEN_CAM_RAM(0x124))

#define NETXEN_PHY_LOCK_ID	(NETXEN_CAM_RAM(0x120))

/* Lock IDs for PHY lock */
#define PHY_LOCK_DRIVER		0x44524956

/* Used for PS PCI Memory access */
#define PCIX_PS_OP_ADDR_LO	(0x10000)
/*   via CRB  (PS side only)     */
#define PCIX_PS_OP_ADDR_HI	(0x10004)

#define PCIX_INT_VECTOR		(0x10100)
#define PCIX_INT_MASK		(0x10104)

#define PCIX_CRB_WINDOW		(0x10210)
#define PCIX_CRB_WINDOW_F0	(0x10210)
#define PCIX_CRB_WINDOW_F1	(0x10230)
#define PCIX_CRB_WINDOW_F2	(0x10250)
#define PCIX_CRB_WINDOW_F3	(0x10270)
#define PCIX_CRB_WINDOW_F4	(0x102ac)
#define PCIX_CRB_WINDOW_F5	(0x102bc)
#define PCIX_CRB_WINDOW_F6	(0x102cc)
#define PCIX_CRB_WINDOW_F7	(0x102dc)
#define PCIE_CRB_WINDOW_REG(func)	(((func) < 4) ? \
		(PCIX_CRB_WINDOW_F0 + (0x20 * (func))) :\
		(PCIX_CRB_WINDOW_F4 + (0x10 * ((func)-4))))

#define PCIX_MN_WINDOW		(0x10200)
#define PCIX_MN_WINDOW_F0	(0x10200)
#define PCIX_MN_WINDOW_F1	(0x10220)
#define PCIX_MN_WINDOW_F2	(0x10240)
#define PCIX_MN_WINDOW_F3	(0x10260)
#define PCIX_MN_WINDOW_F4	(0x102a0)
#define PCIX_MN_WINDOW_F5	(0x102b0)
#define PCIX_MN_WINDOW_F6	(0x102c0)
#define PCIX_MN_WINDOW_F7	(0x102d0)
#define PCIE_MN_WINDOW_REG(func)	(((func) < 4) ? \
		(PCIX_MN_WINDOW_F0 + (0x20 * (func))) :\
		(PCIX_MN_WINDOW_F4 + (0x10 * ((func)-4))))

#define PCIX_SN_WINDOW		(0x10208)
#define PCIX_SN_WINDOW_F0	(0x10208)
#define PCIX_SN_WINDOW_F1	(0x10228)
#define PCIX_SN_WINDOW_F2	(0x10248)
#define PCIX_SN_WINDOW_F3	(0x10268)
#define PCIX_SN_WINDOW_F4	(0x102a8)
#define PCIX_SN_WINDOW_F5	(0x102b8)
#define PCIX_SN_WINDOW_F6	(0x102c8)
#define PCIX_SN_WINDOW_F7	(0x102d8)
#define PCIE_SN_WINDOW_REG(func)	(((func) < 4) ? \
		(PCIX_SN_WINDOW_F0 + (0x20 * (func))) :\
		(PCIX_SN_WINDOW_F4 + (0x10 * ((func)-4))))

#define PCIX_TARGET_STATUS	(0x10118)
#define PCIX_TARGET_STATUS_F1	(0x10160)
#define PCIX_TARGET_STATUS_F2	(0x10164)
#define PCIX_TARGET_STATUS_F3	(0x10168)
#define PCIX_TARGET_STATUS_F4	(0x10360)
#define PCIX_TARGET_STATUS_F5	(0x10364)
#define PCIX_TARGET_STATUS_F6	(0x10368)
#define PCIX_TARGET_STATUS_F7	(0x1036c)

#define PCIX_TARGET_MASK	(0x10128)
#define PCIX_TARGET_MASK_F1	(0x10170)
#define PCIX_TARGET_MASK_F2	(0x10174)
#define PCIX_TARGET_MASK_F3	(0x10178)
#define PCIX_TARGET_MASK_F4	(0x10370)
#define PCIX_TARGET_MASK_F5	(0x10374)
#define PCIX_TARGET_MASK_F6	(0x10378)
#define PCIX_TARGET_MASK_F7	(0x1037c)

#define PCIX_MSI_F0		(0x13000)
#define PCIX_MSI_F1		(0x13004)
#define PCIX_MSI_F2		(0x13008)
#define PCIX_MSI_F3		(0x1300c)
#define PCIX_MSI_F4		(0x13010)
#define PCIX_MSI_F5		(0x13014)
#define PCIX_MSI_F6		(0x13018)
#define PCIX_MSI_F7		(0x1301c)
#define PCIX_MSI_F(i)		(0x13000+((i)*4))

#define PCIX_PS_MEM_SPACE	(0x90000)

#define NETXEN_PCIX_PH_REG(reg)	(NETXEN_CRB_PCIE + (reg))
#define NETXEN_PCIX_PS_REG(reg)	(NETXEN_CRB_PCIX_MD + (reg))

#define NETXEN_PCIE_REG(reg)	(NETXEN_CRB_PCIE + (reg))

#define PCIE_MAX_DMA_XFER_SIZE	(0x1404c)

#define PCIE_DCR		0x00d8

#define PCIE_SEM2_LOCK		(0x1c010)	/* Flash lock   */
#define PCIE_SEM2_UNLOCK	(0x1c014)	/* Flash unlock */
#define PCIE_SEM3_LOCK	  	(0x1c018)	/* Phy lock     */
#define PCIE_SEM3_UNLOCK	(0x1c01c)	/* Phy unlock   */
#define PCIE_SEM5_LOCK		(0x1c028)	/* API lock     */
#define PCIE_SEM5_UNLOCK	(0x1c02c)	/* API unlock   */
#define PCIE_SEM6_LOCK		(0x1c030)	/* sw lock      */
#define PCIE_SEM6_UNLOCK	(0x1c034)	/* sw unlock    */
#define PCIE_SEM7_LOCK		(0x1c038)	/* crb win lock */
#define PCIE_SEM7_UNLOCK	(0x1c03c)	/* crbwin unlock*/

#define PCIE_SETUP_FUNCTION	(0x12040)
#define PCIE_SETUP_FUNCTION2	(0x12048)
#define PCIE_TGT_SPLIT_CHICKEN	(0x12080)
#define PCIE_CHICKEN3		(0x120c8)

#define PCIE_MAX_MASTER_SPLIT	(0x14048)

#define NETXEN_PORT_MODE_NONE		0
#define NETXEN_PORT_MODE_XG		1
#define NETXEN_PORT_MODE_GB		2
#define NETXEN_PORT_MODE_802_3_AP	3
#define NETXEN_PORT_MODE_AUTO_NEG	4
#define NETXEN_PORT_MODE_AUTO_NEG_1G	5
#define NETXEN_PORT_MODE_AUTO_NEG_XG	6
#define NETXEN_PORT_MODE_ADDR		(NETXEN_CAM_RAM(0x24))
#define NETXEN_WOL_PORT_MODE		(NETXEN_CAM_RAM(0x198))

#define NETXEN_CAM_RAM_DMA_WATCHDOG_CTRL		(0x14)

#define	ISR_MSI_INT_TRIGGER(FUNC) (NETXEN_PCIX_PS_REG(PCIX_MSI_F(FUNC)))

/*
 * PCI Interrupt Vector Values.
 */
#define	PCIX_INT_VECTOR_BIT_F0	0x0080
#define	PCIX_INT_VECTOR_BIT_F1	0x0100
#define	PCIX_INT_VECTOR_BIT_F2	0x0200
#define	PCIX_INT_VECTOR_BIT_F3	0x0400
#define	PCIX_INT_VECTOR_BIT_F4	0x0800
#define	PCIX_INT_VECTOR_BIT_F5	0x1000
#define	PCIX_INT_VECTOR_BIT_F6	0x2000
#define	PCIX_INT_VECTOR_BIT_F7	0x4000

struct netxen_legacy_intr_set {
	uint32_t	int_vec_bit;
	uint32_t	tgt_status_reg;
	uint32_t	tgt_mask_reg;
	uint32_t	pci_int_reg;
};

#define	NX_LEGACY_INTR_CONFIG						\
{									\
	{								\
		.int_vec_bit	=	PCIX_INT_VECTOR_BIT_F0,		\
		.tgt_status_reg	=	ISR_INT_TARGET_STATUS,		\
		.tgt_mask_reg	=	ISR_INT_TARGET_MASK,		\
		.pci_int_reg	=	ISR_MSI_INT_TRIGGER(0) },	\
									\
	{								\
		.int_vec_bit	=	PCIX_INT_VECTOR_BIT_F1,		\
		.tgt_status_reg	=	ISR_INT_TARGET_STATUS_F1,	\
		.tgt_mask_reg	=	ISR_INT_TARGET_MASK_F1,		\
		.pci_int_reg	=	ISR_MSI_INT_TRIGGER(1) },	\
									\
	{								\
		.int_vec_bit	=	PCIX_INT_VECTOR_BIT_F2,		\
		.tgt_status_reg	=	ISR_INT_TARGET_STATUS_F2,	\
		.tgt_mask_reg	=	ISR_INT_TARGET_MASK_F2,		\
		.pci_int_reg	=	ISR_MSI_INT_TRIGGER(2) },	\
									\
	{								\
		.int_vec_bit	=	PCIX_INT_VECTOR_BIT_F3,		\
		.tgt_status_reg	=	ISR_INT_TARGET_STATUS_F3,	\
		.tgt_mask_reg	=	ISR_INT_TARGET_MASK_F3,		\
		.pci_int_reg	=	ISR_MSI_INT_TRIGGER(3) },	\
									\
	{								\
		.int_vec_bit	=	PCIX_INT_VECTOR_BIT_F4,		\
		.tgt_status_reg	=	ISR_INT_TARGET_STATUS_F4,	\
		.tgt_mask_reg	=	ISR_INT_TARGET_MASK_F4,		\
		.pci_int_reg	=	ISR_MSI_INT_TRIGGER(4) },	\
									\
	{								\
		.int_vec_bit	=	PCIX_INT_VECTOR_BIT_F5,		\
		.tgt_status_reg	=	ISR_INT_TARGET_STATUS_F5,	\
		.tgt_mask_reg	=	ISR_INT_TARGET_MASK_F5,		\
		.pci_int_reg	=	ISR_MSI_INT_TRIGGER(5) },	\
									\
	{								\
		.int_vec_bit	=	PCIX_INT_VECTOR_BIT_F6,		\
		.tgt_status_reg	=	ISR_INT_TARGET_STATUS_F6,	\
		.tgt_mask_reg	=	ISR_INT_TARGET_MASK_F6,		\
		.pci_int_reg	=	ISR_MSI_INT_TRIGGER(6) },	\
									\
	{								\
		.int_vec_bit	=	PCIX_INT_VECTOR_BIT_F7,		\
		.tgt_status_reg	=	ISR_INT_TARGET_STATUS_F7,	\
		.tgt_mask_reg	=	ISR_INT_TARGET_MASK_F7,		\
		.pci_int_reg	=	ISR_MSI_INT_TRIGGER(7) },	\
}

#endif				/* __NETXEN_NIC_HDR_H_ */
