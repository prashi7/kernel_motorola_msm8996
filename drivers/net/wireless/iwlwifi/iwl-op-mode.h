/******************************************************************************
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2007 - 2012 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110,
 * USA
 *
 * The full GNU General Public License is included in this distribution
 * in the file called LICENSE.GPL.
 *
 * Contact Information:
 *  Intel Linux Wireless <ilw@linux.intel.com>
 * Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497
 *
 * BSD LICENSE
 *
 * Copyright(c) 2005 - 2012 Intel Corporation. All rights reserved.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/
#ifndef __iwl_op_mode_h__
#define __iwl_op_mode_h__

struct iwl_op_mode;
struct iwl_trans;
struct sk_buff;

/**
 * struct iwl_op_mode_ops - op_mode specific operations
 *
 * All the handlers MUST be implemented
 *
 * @start: start the op_mode
 *	May sleep
 * @stop: stop the op_mode
 *	May sleep
 * @free_skb: allows the transport layer to free skbs that haven't been
 *	reclaimed by the op_mode. This can happen when the driver is freed and
 *	there are Tx packets pending in the transport layer.
 *	Must be atomic
 */
struct iwl_op_mode_ops {
	struct iwl_op_mode *(*start)(struct iwl_trans *trans);
	void (*stop)(struct iwl_op_mode *op_mode);
	void (*free_skb)(struct iwl_op_mode *op_mode, struct sk_buff *skb);
};

/**
 * struct iwl_op_mode - operational mode
 *
 * This holds an implementation of the mac80211 / fw API.
 *
 * @ops - pointer to its own ops
 */
struct iwl_op_mode {
	const struct iwl_op_mode_ops *ops;
	const struct iwl_trans *trans;

	char op_mode_specific[0] __aligned(sizeof(void *));
};

static inline void iwl_op_mode_stop(struct iwl_op_mode *op_mode)
{
	op_mode->ops->stop(op_mode);
}

static inline void iwl_op_mode_free_skb(struct iwl_op_mode *op_mode,
					struct sk_buff *skb)
{
	op_mode->ops->free_skb(op_mode, skb);
}

/*****************************************************
* Op mode layers implementations
******************************************************/
extern const struct iwl_op_mode_ops iwl_dvm_ops;

#endif /* __iwl_op_mode_h__ */
