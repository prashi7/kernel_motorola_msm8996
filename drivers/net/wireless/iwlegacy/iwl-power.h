/******************************************************************************
 *
 * Copyright(c) 2007 - 2011 Intel Corporation. All rights reserved.
 *
 * Portions of this file are derived from the ipw3945 project, as well
 * as portions of the ieee80211 subsystem header files.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 *  Intel Linux Wireless <ilw@linux.intel.com>
 * Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497
 *****************************************************************************/
#ifndef __il_power_setting_h__
#define __il_power_setting_h__

#include "commands.h"

enum il_power_level {
	IL_POWER_IDX_1,
	IL_POWER_IDX_2,
	IL_POWER_IDX_3,
	IL_POWER_IDX_4,
	IL_POWER_IDX_5,
	IL_POWER_NUM
};

struct il_power_mgr {
	struct il_powertable_cmd sleep_cmd;
	struct il_powertable_cmd sleep_cmd_next;
	int debug_sleep_level_override;
	bool pci_pm;
};

int
il_power_set_mode(struct il_priv *il, struct il_powertable_cmd *cmd,
		       bool force);
int il_power_update_mode(struct il_priv *il, bool force);
void il_power_initialize(struct il_priv *il);

#endif  /* __il_power_setting_h__ */
