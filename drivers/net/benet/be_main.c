/*
 * Copyright (C) 2005 - 2011 Emulex
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.  The full GNU General
 * Public License is included in this distribution in the file called COPYING.
 *
 * Contact Information:
 * linux-drivers@emulex.com
 *
 * Emulex
 * 3333 Susan Street
 * Costa Mesa, CA 92626
 */

#include <linux/prefetch.h>
#include "be.h"
#include "be_cmds.h"
#include <asm/div64.h>

MODULE_VERSION(DRV_VER);
MODULE_DEVICE_TABLE(pci, be_dev_ids);
MODULE_DESCRIPTION(DRV_DESC " " DRV_VER);
MODULE_AUTHOR("ServerEngines Corporation");
MODULE_LICENSE("GPL");

static ushort rx_frag_size = 2048;
static unsigned int num_vfs;
module_param(rx_frag_size, ushort, S_IRUGO);
module_param(num_vfs, uint, S_IRUGO);
MODULE_PARM_DESC(rx_frag_size, "Size of a fragment that holds rcvd data.");
MODULE_PARM_DESC(num_vfs, "Number of PCI VFs to initialize");

static DEFINE_PCI_DEVICE_TABLE(be_dev_ids) = {
	{ PCI_DEVICE(BE_VENDOR_ID, BE_DEVICE_ID1) },
	{ PCI_DEVICE(BE_VENDOR_ID, BE_DEVICE_ID2) },
	{ PCI_DEVICE(BE_VENDOR_ID, OC_DEVICE_ID1) },
	{ PCI_DEVICE(BE_VENDOR_ID, OC_DEVICE_ID2) },
	{ PCI_DEVICE(EMULEX_VENDOR_ID, OC_DEVICE_ID3)},
	{ PCI_DEVICE(EMULEX_VENDOR_ID, OC_DEVICE_ID4)},
	{ 0 }
};
MODULE_DEVICE_TABLE(pci, be_dev_ids);
/* UE Status Low CSR */
static char *ue_status_low_desc[] = {
	"CEV",
	"CTX",
	"DBUF",
	"ERX",
	"Host",
	"MPU",
	"NDMA",
	"PTC ",
	"RDMA ",
	"RXF ",
	"RXIPS ",
	"RXULP0 ",
	"RXULP1 ",
	"RXULP2 ",
	"TIM ",
	"TPOST ",
	"TPRE ",
	"TXIPS ",
	"TXULP0 ",
	"TXULP1 ",
	"UC ",
	"WDMA ",
	"TXULP2 ",
	"HOST1 ",
	"P0_OB_LINK ",
	"P1_OB_LINK ",
	"HOST_GPIO ",
	"MBOX ",
	"AXGMAC0",
	"AXGMAC1",
	"JTAG",
	"MPU_INTPEND"
};
/* UE Status High CSR */
static char *ue_status_hi_desc[] = {
	"LPCMEMHOST",
	"MGMT_MAC",
	"PCS0ONLINE",
	"MPU_IRAM",
	"PCS1ONLINE",
	"PCTL0",
	"PCTL1",
	"PMEM",
	"RR",
	"TXPB",
	"RXPP",
	"XAUI",
	"TXP",
	"ARM",
	"IPC",
	"HOST2",
	"HOST3",
	"HOST4",
	"HOST5",
	"HOST6",
	"HOST7",
	"HOST8",
	"HOST9",
	"NETC"
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown"
};

static void be_queue_free(struct be_adapter *adapter, struct be_queue_info *q)
{
	struct be_dma_mem *mem = &q->dma_mem;
	if (mem->va)
		dma_free_coherent(&adapter->pdev->dev, mem->size, mem->va,
				  mem->dma);
}

static int be_queue_alloc(struct be_adapter *adapter, struct be_queue_info *q,
		u16 len, u16 entry_size)
{
	struct be_dma_mem *mem = &q->dma_mem;

	memset(q, 0, sizeof(*q));
	q->len = len;
	q->entry_size = entry_size;
	mem->size = len * entry_size;
	mem->va = dma_alloc_coherent(&adapter->pdev->dev, mem->size, &mem->dma,
				     GFP_KERNEL);
	if (!mem->va)
		return -1;
	memset(mem->va, 0, mem->size);
	return 0;
}

static void be_intr_set(struct be_adapter *adapter, bool enable)
{
	u8 __iomem *addr = adapter->pcicfg + PCICFG_MEMBAR_CTRL_INT_CTRL_OFFSET;
	u32 reg = ioread32(addr);
	u32 enabled = reg & MEMBAR_CTRL_INT_CTRL_HOSTINTR_MASK;

	if (adapter->eeh_err)
		return;

	if (!enabled && enable)
		reg |= MEMBAR_CTRL_INT_CTRL_HOSTINTR_MASK;
	else if (enabled && !enable)
		reg &= ~MEMBAR_CTRL_INT_CTRL_HOSTINTR_MASK;
	else
		return;

	iowrite32(reg, addr);
}

static void be_rxq_notify(struct be_adapter *adapter, u16 qid, u16 posted)
{
	u32 val = 0;
	val |= qid & DB_RQ_RING_ID_MASK;
	val |= posted << DB_RQ_NUM_POSTED_SHIFT;

	wmb();
	iowrite32(val, adapter->db + DB_RQ_OFFSET);
}

static void be_txq_notify(struct be_adapter *adapter, u16 qid, u16 posted)
{
	u32 val = 0;
	val |= qid & DB_TXULP_RING_ID_MASK;
	val |= (posted & DB_TXULP_NUM_POSTED_MASK) << DB_TXULP_NUM_POSTED_SHIFT;

	wmb();
	iowrite32(val, adapter->db + DB_TXULP1_OFFSET);
}

static void be_eq_notify(struct be_adapter *adapter, u16 qid,
		bool arm, bool clear_int, u16 num_popped)
{
	u32 val = 0;
	val |= qid & DB_EQ_RING_ID_MASK;
	val |= ((qid & DB_EQ_RING_ID_EXT_MASK) <<
			DB_EQ_RING_ID_EXT_MASK_SHIFT);

	if (adapter->eeh_err)
		return;

	if (arm)
		val |= 1 << DB_EQ_REARM_SHIFT;
	if (clear_int)
		val |= 1 << DB_EQ_CLR_SHIFT;
	val |= 1 << DB_EQ_EVNT_SHIFT;
	val |= num_popped << DB_EQ_NUM_POPPED_SHIFT;
	iowrite32(val, adapter->db + DB_EQ_OFFSET);
}

void be_cq_notify(struct be_adapter *adapter, u16 qid, bool arm, u16 num_popped)
{
	u32 val = 0;
	val |= qid & DB_CQ_RING_ID_MASK;
	val |= ((qid & DB_CQ_RING_ID_EXT_MASK) <<
			DB_CQ_RING_ID_EXT_MASK_SHIFT);

	if (adapter->eeh_err)
		return;

	if (arm)
		val |= 1 << DB_CQ_REARM_SHIFT;
	val |= num_popped << DB_CQ_NUM_POPPED_SHIFT;
	iowrite32(val, adapter->db + DB_CQ_OFFSET);
}

static int be_mac_addr_set(struct net_device *netdev, void *p)
{
	struct be_adapter *adapter = netdev_priv(netdev);
	struct sockaddr *addr = p;
	int status = 0;

	if (!is_valid_ether_addr(addr->sa_data))
		return -EADDRNOTAVAIL;

	/* MAC addr configuration will be done in hardware for VFs
	 * by their corresponding PFs. Just copy to netdev addr here
	 */
	if (!be_physfn(adapter))
		goto netdev_addr;

	status = be_cmd_pmac_del(adapter, adapter->if_handle,
				adapter->pmac_id, 0);
	if (status)
		return status;

	status = be_cmd_pmac_add(adapter, (u8 *)addr->sa_data,
				adapter->if_handle, &adapter->pmac_id, 0);
netdev_addr:
	if (!status)
		memcpy(netdev->dev_addr, addr->sa_data, netdev->addr_len);

	return status;
}

static void populate_be2_stats(struct be_adapter *adapter)
{

	struct be_drv_stats *drvs = &adapter->drv_stats;
	struct be_pmem_stats *pmem_sts = be_pmem_stats_from_cmd(adapter);
	struct be_port_rxf_stats_v0 *port_stats =
		be_port_rxf_stats_from_cmd(adapter);
	struct be_rxf_stats_v0 *rxf_stats =
		be_rxf_stats_from_cmd(adapter);

	drvs->rx_pause_frames = port_stats->rx_pause_frames;
	drvs->rx_crc_errors = port_stats->rx_crc_errors;
	drvs->rx_control_frames = port_stats->rx_control_frames;
	drvs->rx_in_range_errors = port_stats->rx_in_range_errors;
	drvs->rx_frame_too_long = port_stats->rx_frame_too_long;
	drvs->rx_dropped_runt = port_stats->rx_dropped_runt;
	drvs->rx_ip_checksum_errs = port_stats->rx_ip_checksum_errs;
	drvs->rx_tcp_checksum_errs = port_stats->rx_tcp_checksum_errs;
	drvs->rx_udp_checksum_errs = port_stats->rx_udp_checksum_errs;
	drvs->rxpp_fifo_overflow_drop = port_stats->rx_fifo_overflow;
	drvs->rx_dropped_tcp_length = port_stats->rx_dropped_tcp_length;
	drvs->rx_dropped_too_small = port_stats->rx_dropped_too_small;
	drvs->rx_dropped_too_short = port_stats->rx_dropped_too_short;
	drvs->rx_out_range_errors = port_stats->rx_out_range_errors;
	drvs->rx_input_fifo_overflow_drop =
		port_stats->rx_input_fifo_overflow;
	drvs->rx_dropped_header_too_small =
		port_stats->rx_dropped_header_too_small;
	drvs->rx_address_match_errors =
		port_stats->rx_address_match_errors;
	drvs->rx_alignment_symbol_errors =
		port_stats->rx_alignment_symbol_errors;

	drvs->tx_pauseframes = port_stats->tx_pauseframes;
	drvs->tx_controlframes = port_stats->tx_controlframes;

	if (adapter->port_num)
		drvs->jabber_events =
			rxf_stats->port1_jabber_events;
	else
		drvs->jabber_events =
			rxf_stats->port0_jabber_events;
	drvs->rx_drops_no_pbuf = rxf_stats->rx_drops_no_pbuf;
	drvs->rx_drops_no_txpb = rxf_stats->rx_drops_no_txpb;
	drvs->rx_drops_no_erx_descr = rxf_stats->rx_drops_no_erx_descr;
	drvs->rx_drops_invalid_ring = rxf_stats->rx_drops_invalid_ring;
	drvs->forwarded_packets = rxf_stats->forwarded_packets;
	drvs->rx_drops_mtu = rxf_stats->rx_drops_mtu;
	drvs->rx_drops_no_tpre_descr =
		rxf_stats->rx_drops_no_tpre_descr;
	drvs->rx_drops_too_many_frags =
		rxf_stats->rx_drops_too_many_frags;
	adapter->drv_stats.eth_red_drops = pmem_sts->eth_red_drops;
}

static void populate_be3_stats(struct be_adapter *adapter)
{
	struct be_drv_stats *drvs = &adapter->drv_stats;
	struct be_pmem_stats *pmem_sts = be_pmem_stats_from_cmd(adapter);

	struct be_rxf_stats_v1 *rxf_stats =
		be_rxf_stats_from_cmd(adapter);
	struct be_port_rxf_stats_v1 *port_stats =
		be_port_rxf_stats_from_cmd(adapter);

	drvs->rx_priority_pause_frames = 0;
	drvs->pmem_fifo_overflow_drop = 0;
	drvs->rx_pause_frames = port_stats->rx_pause_frames;
	drvs->rx_crc_errors = port_stats->rx_crc_errors;
	drvs->rx_control_frames = port_stats->rx_control_frames;
	drvs->rx_in_range_errors = port_stats->rx_in_range_errors;
	drvs->rx_frame_too_long = port_stats->rx_frame_too_long;
	drvs->rx_dropped_runt = port_stats->rx_dropped_runt;
	drvs->rx_ip_checksum_errs = port_stats->rx_ip_checksum_errs;
	drvs->rx_tcp_checksum_errs = port_stats->rx_tcp_checksum_errs;
	drvs->rx_udp_checksum_errs = port_stats->rx_udp_checksum_errs;
	drvs->rx_dropped_tcp_length = port_stats->rx_dropped_tcp_length;
	drvs->rx_dropped_too_small = port_stats->rx_dropped_too_small;
	drvs->rx_dropped_too_short = port_stats->rx_dropped_too_short;
	drvs->rx_out_range_errors = port_stats->rx_out_range_errors;
	drvs->rx_dropped_header_too_small =
		port_stats->rx_dropped_header_too_small;
	drvs->rx_input_fifo_overflow_drop =
		port_stats->rx_input_fifo_overflow_drop;
	drvs->rx_address_match_errors =
		port_stats->rx_address_match_errors;
	drvs->rx_alignment_symbol_errors =
		port_stats->rx_alignment_symbol_errors;
	drvs->rxpp_fifo_overflow_drop =
		port_stats->rxpp_fifo_overflow_drop;
	drvs->tx_pauseframes = port_stats->tx_pauseframes;
	drvs->tx_controlframes = port_stats->tx_controlframes;
	drvs->jabber_events = port_stats->jabber_events;
	drvs->rx_drops_no_pbuf = rxf_stats->rx_drops_no_pbuf;
	drvs->rx_drops_no_txpb = rxf_stats->rx_drops_no_txpb;
	drvs->rx_drops_no_erx_descr = rxf_stats->rx_drops_no_erx_descr;
	drvs->rx_drops_invalid_ring = rxf_stats->rx_drops_invalid_ring;
	drvs->forwarded_packets = rxf_stats->forwarded_packets;
	drvs->rx_drops_mtu = rxf_stats->rx_drops_mtu;
	drvs->rx_drops_no_tpre_descr =
		rxf_stats->rx_drops_no_tpre_descr;
	drvs->rx_drops_too_many_frags =
		rxf_stats->rx_drops_too_many_frags;
	adapter->drv_stats.eth_red_drops = pmem_sts->eth_red_drops;
}

static void populate_lancer_stats(struct be_adapter *adapter)
{

	struct be_drv_stats *drvs = &adapter->drv_stats;
	struct lancer_cmd_pport_stats *pport_stats = pport_stats_from_cmd
						(adapter);
	drvs->rx_priority_pause_frames = 0;
	drvs->pmem_fifo_overflow_drop = 0;
	drvs->rx_pause_frames =
		make_64bit_val(pport_stats->rx_pause_frames_hi,
				 pport_stats->rx_pause_frames_lo);
	drvs->rx_crc_errors = make_64bit_val(pport_stats->rx_crc_errors_hi,
						pport_stats->rx_crc_errors_lo);
	drvs->rx_control_frames =
			make_64bit_val(pport_stats->rx_control_frames_hi,
			pport_stats->rx_control_frames_lo);
	drvs->rx_in_range_errors = pport_stats->rx_in_range_errors;
	drvs->rx_frame_too_long =
		make_64bit_val(pport_stats->rx_internal_mac_errors_hi,
					pport_stats->rx_frames_too_long_lo);
	drvs->rx_dropped_runt = pport_stats->rx_dropped_runt;
	drvs->rx_ip_checksum_errs = pport_stats->rx_ip_checksum_errors;
	drvs->rx_tcp_checksum_errs = pport_stats->rx_tcp_checksum_errors;
	drvs->rx_udp_checksum_errs = pport_stats->rx_udp_checksum_errors;
	drvs->rx_dropped_tcp_length =
				pport_stats->rx_dropped_invalid_tcp_length;
	drvs->rx_dropped_too_small = pport_stats->rx_dropped_too_small;
	drvs->rx_dropped_too_short = pport_stats->rx_dropped_too_short;
	drvs->rx_out_range_errors = pport_stats->rx_out_of_range_errors;
	drvs->rx_dropped_header_too_small =
				pport_stats->rx_dropped_header_too_small;
	drvs->rx_input_fifo_overflow_drop = pport_stats->rx_fifo_overflow;
	drvs->rx_address_match_errors = pport_stats->rx_address_match_errors;
	drvs->rx_alignment_symbol_errors =
		make_64bit_val(pport_stats->rx_symbol_errors_hi,
				pport_stats->rx_symbol_errors_lo);
	drvs->rxpp_fifo_overflow_drop = pport_stats->rx_fifo_overflow;
	drvs->tx_pauseframes = make_64bit_val(pport_stats->tx_pause_frames_hi,
					pport_stats->tx_pause_frames_lo);
	drvs->tx_controlframes =
		make_64bit_val(pport_stats->tx_control_frames_hi,
				pport_stats->tx_control_frames_lo);
	drvs->jabber_events = pport_stats->rx_jabbers;
	drvs->rx_drops_no_pbuf = 0;
	drvs->rx_drops_no_txpb = 0;
	drvs->rx_drops_no_erx_descr = 0;
	drvs->rx_drops_invalid_ring = pport_stats->rx_drops_invalid_queue;
	drvs->forwarded_packets = make_64bit_val(pport_stats->num_forwards_hi,
						pport_stats->num_forwards_lo);
	drvs->rx_drops_mtu = make_64bit_val(pport_stats->rx_drops_mtu_hi,
						pport_stats->rx_drops_mtu_lo);
	drvs->rx_drops_no_tpre_descr = 0;
	drvs->rx_drops_too_many_frags =
		make_64bit_val(pport_stats->rx_drops_too_many_frags_hi,
				pport_stats->rx_drops_too_many_frags_lo);
}

void be_parse_stats(struct be_adapter *adapter)
{
	if (adapter->generation == BE_GEN3) {
		if (lancer_chip(adapter))
			populate_lancer_stats(adapter);
		 else
			populate_be3_stats(adapter);
	} else {
		populate_be2_stats(adapter);
	}
}

void netdev_stats_update(struct be_adapter *adapter)
{
	struct be_drv_stats *drvs = &adapter->drv_stats;
	struct net_device_stats *dev_stats = &adapter->netdev->stats;
	struct be_rx_obj *rxo;
	struct be_tx_obj *txo;
	unsigned long pkts = 0, bytes = 0, mcast = 0, drops = 0;
	int i;

	for_all_rx_queues(adapter, rxo, i) {
		pkts += rx_stats(rxo)->rx_pkts;
		bytes += rx_stats(rxo)->rx_bytes;
		mcast += rx_stats(rxo)->rx_mcast_pkts;
		/*  no space in linux buffers: best possible approximation */
		if (adapter->generation == BE_GEN3) {
			if (!(lancer_chip(adapter))) {
				struct be_erx_stats_v1 *erx =
					be_erx_stats_from_cmd(adapter);
				drops += erx->rx_drops_no_fragments[rxo->q.id];
			}
		} else {
			struct be_erx_stats_v0 *erx =
					be_erx_stats_from_cmd(adapter);
			drops += erx->rx_drops_no_fragments[rxo->q.id];
		}
	}
	dev_stats->rx_packets = pkts;
	dev_stats->rx_bytes = bytes;
	dev_stats->multicast = mcast;
	dev_stats->rx_dropped = drops;

	pkts = bytes = 0;
	for_all_tx_queues(adapter, txo, i) {
		pkts += tx_stats(txo)->be_tx_pkts;
		bytes += tx_stats(txo)->be_tx_bytes;
	}
	dev_stats->tx_packets = pkts;
	dev_stats->tx_bytes = bytes;

	/* bad pkts received */
	dev_stats->rx_errors = drvs->rx_crc_errors +
		drvs->rx_alignment_symbol_errors +
		drvs->rx_in_range_errors +
		drvs->rx_out_range_errors +
		drvs->rx_frame_too_long +
		drvs->rx_dropped_too_small +
		drvs->rx_dropped_too_short +
		drvs->rx_dropped_header_too_small +
		drvs->rx_dropped_tcp_length +
		drvs->rx_dropped_runt +
		drvs->rx_tcp_checksum_errs +
		drvs->rx_ip_checksum_errs +
		drvs->rx_udp_checksum_errs;

	/* detailed rx errors */
	dev_stats->rx_length_errors = drvs->rx_in_range_errors +
		drvs->rx_out_range_errors +
		drvs->rx_frame_too_long;

	dev_stats->rx_crc_errors = drvs->rx_crc_errors;

	/* frame alignment errors */
	dev_stats->rx_frame_errors = drvs->rx_alignment_symbol_errors;

	/* receiver fifo overrun */
	/* drops_no_pbuf is no per i/f, it's per BE card */
	dev_stats->rx_fifo_errors = drvs->rxpp_fifo_overflow_drop +
				drvs->rx_input_fifo_overflow_drop +
				drvs->rx_drops_no_pbuf;
}

void be_link_status_update(struct be_adapter *adapter, bool link_up)
{
	struct net_device *netdev = adapter->netdev;

	/* If link came up or went down */
	if (adapter->link_up != link_up) {
		adapter->link_speed = -1;
		if (link_up) {
			netif_carrier_on(netdev);
			printk(KERN_INFO "%s: Link up\n", netdev->name);
		} else {
			netif_carrier_off(netdev);
			printk(KERN_INFO "%s: Link down\n", netdev->name);
		}
		adapter->link_up = link_up;
	}
}

/* Update the EQ delay n BE based on the RX frags consumed / sec */
static void be_rx_eqd_update(struct be_adapter *adapter, struct be_rx_obj *rxo)
{
	struct be_eq_obj *rx_eq = &rxo->rx_eq;
	struct be_rx_stats *stats = &rxo->stats;
	ulong now = jiffies;
	u32 eqd;

	if (!rx_eq->enable_aic)
		return;

	/* Wrapped around */
	if (time_before(now, stats->rx_fps_jiffies)) {
		stats->rx_fps_jiffies = now;
		return;
	}

	/* Update once a second */
	if ((now - stats->rx_fps_jiffies) < HZ)
		return;

	stats->rx_fps = (stats->rx_frags - stats->prev_rx_frags) /
			((now - stats->rx_fps_jiffies) / HZ);

	stats->rx_fps_jiffies = now;
	stats->prev_rx_frags = stats->rx_frags;
	eqd = stats->rx_fps / 110000;
	eqd = eqd << 3;
	if (eqd > rx_eq->max_eqd)
		eqd = rx_eq->max_eqd;
	if (eqd < rx_eq->min_eqd)
		eqd = rx_eq->min_eqd;
	if (eqd < 10)
		eqd = 0;
	if (eqd != rx_eq->cur_eqd)
		be_cmd_modify_eqd(adapter, rx_eq->q.id, eqd);

	rx_eq->cur_eqd = eqd;
}

static u32 be_calc_rate(u64 bytes, unsigned long ticks)
{
	u64 rate = bytes;

	do_div(rate, ticks / HZ);
	rate <<= 3;			/* bytes/sec -> bits/sec */
	do_div(rate, 1000000ul);	/* MB/Sec */

	return rate;
}

static void be_tx_rate_update(struct be_tx_obj *txo)
{
	struct be_tx_stats *stats = tx_stats(txo);
	ulong now = jiffies;

	/* Wrapped around? */
	if (time_before(now, stats->be_tx_jiffies)) {
		stats->be_tx_jiffies = now;
		return;
	}

	/* Update tx rate once in two seconds */
	if ((now - stats->be_tx_jiffies) > 2 * HZ) {
		stats->be_tx_rate = be_calc_rate(stats->be_tx_bytes
						  - stats->be_tx_bytes_prev,
						 now - stats->be_tx_jiffies);
		stats->be_tx_jiffies = now;
		stats->be_tx_bytes_prev = stats->be_tx_bytes;
	}
}

static void be_tx_stats_update(struct be_tx_obj *txo,
			u32 wrb_cnt, u32 copied, u32 gso_segs, bool stopped)
{
	struct be_tx_stats *stats = tx_stats(txo);

	stats->be_tx_reqs++;
	stats->be_tx_wrbs += wrb_cnt;
	stats->be_tx_bytes += copied;
	stats->be_tx_pkts += (gso_segs ? gso_segs : 1);
	if (stopped)
		stats->be_tx_stops++;
}

/* Determine number of WRB entries needed to xmit data in an skb */
static u32 wrb_cnt_for_skb(struct be_adapter *adapter, struct sk_buff *skb,
								bool *dummy)
{
	int cnt = (skb->len > skb->data_len);

	cnt += skb_shinfo(skb)->nr_frags;

	/* to account for hdr wrb */
	cnt++;
	if (lancer_chip(adapter) || !(cnt & 1)) {
		*dummy = false;
	} else {
		/* add a dummy to make it an even num */
		cnt++;
		*dummy = true;
	}
	BUG_ON(cnt > BE_MAX_TX_FRAG_COUNT);
	return cnt;
}

static inline void wrb_fill(struct be_eth_wrb *wrb, u64 addr, int len)
{
	wrb->frag_pa_hi = upper_32_bits(addr);
	wrb->frag_pa_lo = addr & 0xFFFFFFFF;
	wrb->frag_len = len & ETH_WRB_FRAG_LEN_MASK;
}

static void wrb_fill_hdr(struct be_adapter *adapter, struct be_eth_hdr_wrb *hdr,
		struct sk_buff *skb, u32 wrb_cnt, u32 len)
{
	u8 vlan_prio = 0;
	u16 vlan_tag = 0;

	memset(hdr, 0, sizeof(*hdr));

	AMAP_SET_BITS(struct amap_eth_hdr_wrb, crc, hdr, 1);

	if (skb_is_gso(skb)) {
		AMAP_SET_BITS(struct amap_eth_hdr_wrb, lso, hdr, 1);
		AMAP_SET_BITS(struct amap_eth_hdr_wrb, lso_mss,
			hdr, skb_shinfo(skb)->gso_size);
		if (skb_is_gso_v6(skb) && !lancer_chip(adapter))
			AMAP_SET_BITS(struct amap_eth_hdr_wrb, lso6, hdr, 1);
		if (lancer_chip(adapter) && adapter->sli_family  ==
							LANCER_A0_SLI_FAMILY) {
			AMAP_SET_BITS(struct amap_eth_hdr_wrb, ipcs, hdr, 1);
			if (is_tcp_pkt(skb))
				AMAP_SET_BITS(struct amap_eth_hdr_wrb,
								tcpcs, hdr, 1);
			else if (is_udp_pkt(skb))
				AMAP_SET_BITS(struct amap_eth_hdr_wrb,
								udpcs, hdr, 1);
		}
	} else if (skb->ip_summed == CHECKSUM_PARTIAL) {
		if (is_tcp_pkt(skb))
			AMAP_SET_BITS(struct amap_eth_hdr_wrb, tcpcs, hdr, 1);
		else if (is_udp_pkt(skb))
			AMAP_SET_BITS(struct amap_eth_hdr_wrb, udpcs, hdr, 1);
	}

	if (adapter->vlan_grp && vlan_tx_tag_present(skb)) {
		AMAP_SET_BITS(struct amap_eth_hdr_wrb, vlan, hdr, 1);
		vlan_tag = vlan_tx_tag_get(skb);
		vlan_prio = (vlan_tag & VLAN_PRIO_MASK) >> VLAN_PRIO_SHIFT;
		/* If vlan priority provided by OS is NOT in available bmap */
		if (!(adapter->vlan_prio_bmap & (1 << vlan_prio)))
			vlan_tag = (vlan_tag & ~VLAN_PRIO_MASK) |
					adapter->recommended_prio;
		AMAP_SET_BITS(struct amap_eth_hdr_wrb, vlan_tag, hdr, vlan_tag);
	}

	AMAP_SET_BITS(struct amap_eth_hdr_wrb, event, hdr, 1);
	AMAP_SET_BITS(struct amap_eth_hdr_wrb, complete, hdr, 1);
	AMAP_SET_BITS(struct amap_eth_hdr_wrb, num_wrb, hdr, wrb_cnt);
	AMAP_SET_BITS(struct amap_eth_hdr_wrb, len, hdr, len);
}

static void unmap_tx_frag(struct device *dev, struct be_eth_wrb *wrb,
		bool unmap_single)
{
	dma_addr_t dma;

	be_dws_le_to_cpu(wrb, sizeof(*wrb));

	dma = (u64)wrb->frag_pa_hi << 32 | (u64)wrb->frag_pa_lo;
	if (wrb->frag_len) {
		if (unmap_single)
			dma_unmap_single(dev, dma, wrb->frag_len,
					 DMA_TO_DEVICE);
		else
			dma_unmap_page(dev, dma, wrb->frag_len, DMA_TO_DEVICE);
	}
}

static int make_tx_wrbs(struct be_adapter *adapter, struct be_queue_info *txq,
		struct sk_buff *skb, u32 wrb_cnt, bool dummy_wrb)
{
	dma_addr_t busaddr;
	int i, copied = 0;
	struct device *dev = &adapter->pdev->dev;
	struct sk_buff *first_skb = skb;
	struct be_eth_wrb *wrb;
	struct be_eth_hdr_wrb *hdr;
	bool map_single = false;
	u16 map_head;

	hdr = queue_head_node(txq);
	queue_head_inc(txq);
	map_head = txq->head;

	if (skb->len > skb->data_len) {
		int len = skb_headlen(skb);
		busaddr = dma_map_single(dev, skb->data, len, DMA_TO_DEVICE);
		if (dma_mapping_error(dev, busaddr))
			goto dma_err;
		map_single = true;
		wrb = queue_head_node(txq);
		wrb_fill(wrb, busaddr, len);
		be_dws_cpu_to_le(wrb, sizeof(*wrb));
		queue_head_inc(txq);
		copied += len;
	}

	for (i = 0; i < skb_shinfo(skb)->nr_frags; i++) {
		struct skb_frag_struct *frag =
			&skb_shinfo(skb)->frags[i];
		busaddr = dma_map_page(dev, frag->page, frag->page_offset,
				       frag->size, DMA_TO_DEVICE);
		if (dma_mapping_error(dev, busaddr))
			goto dma_err;
		wrb = queue_head_node(txq);
		wrb_fill(wrb, busaddr, frag->size);
		be_dws_cpu_to_le(wrb, sizeof(*wrb));
		queue_head_inc(txq);
		copied += frag->size;
	}

	if (dummy_wrb) {
		wrb = queue_head_node(txq);
		wrb_fill(wrb, 0, 0);
		be_dws_cpu_to_le(wrb, sizeof(*wrb));
		queue_head_inc(txq);
	}

	wrb_fill_hdr(adapter, hdr, first_skb, wrb_cnt, copied);
	be_dws_cpu_to_le(hdr, sizeof(*hdr));

	return copied;
dma_err:
	txq->head = map_head;
	while (copied) {
		wrb = queue_head_node(txq);
		unmap_tx_frag(dev, wrb, map_single);
		map_single = false;
		copied -= wrb->frag_len;
		queue_head_inc(txq);
	}
	return 0;
}

static netdev_tx_t be_xmit(struct sk_buff *skb,
			struct net_device *netdev)
{
	struct be_adapter *adapter = netdev_priv(netdev);
	struct be_tx_obj *txo = &adapter->tx_obj[skb_get_queue_mapping(skb)];
	struct be_queue_info *txq = &txo->q;
	u32 wrb_cnt = 0, copied = 0;
	u32 start = txq->head;
	bool dummy_wrb, stopped = false;

	wrb_cnt = wrb_cnt_for_skb(adapter, skb, &dummy_wrb);

	copied = make_tx_wrbs(adapter, txq, skb, wrb_cnt, dummy_wrb);
	if (copied) {
		/* record the sent skb in the sent_skb table */
		BUG_ON(txo->sent_skb_list[start]);
		txo->sent_skb_list[start] = skb;

		/* Ensure txq has space for the next skb; Else stop the queue
		 * *BEFORE* ringing the tx doorbell, so that we serialze the
		 * tx compls of the current transmit which'll wake up the queue
		 */
		atomic_add(wrb_cnt, &txq->used);
		if ((BE_MAX_TX_FRAG_COUNT + atomic_read(&txq->used)) >=
								txq->len) {
			netif_stop_subqueue(netdev, skb_get_queue_mapping(skb));
			stopped = true;
		}

		be_txq_notify(adapter, txq->id, wrb_cnt);

		be_tx_stats_update(txo, wrb_cnt, copied,
				skb_shinfo(skb)->gso_segs, stopped);
	} else {
		txq->head = start;
		dev_kfree_skb_any(skb);
	}
	return NETDEV_TX_OK;
}

static int be_change_mtu(struct net_device *netdev, int new_mtu)
{
	struct be_adapter *adapter = netdev_priv(netdev);
	if (new_mtu < BE_MIN_MTU ||
			new_mtu > (BE_MAX_JUMBO_FRAME_SIZE -
					(ETH_HLEN + ETH_FCS_LEN))) {
		dev_info(&adapter->pdev->dev,
			"MTU must be between %d and %d bytes\n",
			BE_MIN_MTU,
			(BE_MAX_JUMBO_FRAME_SIZE - (ETH_HLEN + ETH_FCS_LEN)));
		return -EINVAL;
	}
	dev_info(&adapter->pdev->dev, "MTU changed from %d to %d bytes\n",
			netdev->mtu, new_mtu);
	netdev->mtu = new_mtu;
	return 0;
}

/*
 * A max of 64 (BE_NUM_VLANS_SUPPORTED) vlans can be configured in BE.
 * If the user configures more, place BE in vlan promiscuous mode.
 */
static int be_vid_config(struct be_adapter *adapter, bool vf, u32 vf_num)
{
	u16 vtag[BE_NUM_VLANS_SUPPORTED];
	u16 ntags = 0, i;
	int status = 0;
	u32 if_handle;

	if (vf) {
		if_handle = adapter->vf_cfg[vf_num].vf_if_handle;
		vtag[0] = cpu_to_le16(adapter->vf_cfg[vf_num].vf_vlan_tag);
		status = be_cmd_vlan_config(adapter, if_handle, vtag, 1, 1, 0);
	}

	if (adapter->vlans_added <= adapter->max_vlans)  {
		/* Construct VLAN Table to give to HW */
		for (i = 0; i < VLAN_N_VID; i++) {
			if (adapter->vlan_tag[i]) {
				vtag[ntags] = cpu_to_le16(i);
				ntags++;
			}
		}
		status = be_cmd_vlan_config(adapter, adapter->if_handle,
					vtag, ntags, 1, 0);
	} else {
		status = be_cmd_vlan_config(adapter, adapter->if_handle,
					NULL, 0, 1, 1);
	}

	return status;
}

static void be_vlan_register(struct net_device *netdev, struct vlan_group *grp)
{
	struct be_adapter *adapter = netdev_priv(netdev);

	adapter->vlan_grp = grp;
}

static void be_vlan_add_vid(struct net_device *netdev, u16 vid)
{
	struct be_adapter *adapter = netdev_priv(netdev);

	adapter->vlans_added++;
	if (!be_physfn(adapter))
		return;

	adapter->vlan_tag[vid] = 1;
	if (adapter->vlans_added <= (adapter->max_vlans + 1))
		be_vid_config(adapter, false, 0);
}

static void be_vlan_rem_vid(struct net_device *netdev, u16 vid)
{
	struct be_adapter *adapter = netdev_priv(netdev);

	adapter->vlans_added--;
	vlan_group_set_device(adapter->vlan_grp, vid, NULL);

	if (!be_physfn(adapter))
		return;

	adapter->vlan_tag[vid] = 0;
	if (adapter->vlans_added <= adapter->max_vlans)
		be_vid_config(adapter, false, 0);
}

static void be_set_multicast_list(struct net_device *netdev)
{
	struct be_adapter *adapter = netdev_priv(netdev);

	if (netdev->flags & IFF_PROMISC) {
		be_cmd_promiscuous_config(adapter, true);
		adapter->promiscuous = true;
		goto done;
	}

	/* BE was previously in promiscuous mode; disable it */
	if (adapter->promiscuous) {
		adapter->promiscuous = false;
		be_cmd_promiscuous_config(adapter, false);
	}

	/* Enable multicast promisc if num configured exceeds what we support */
	if (netdev->flags & IFF_ALLMULTI ||
	    netdev_mc_count(netdev) > BE_MAX_MC) {
		be_cmd_multicast_set(adapter, adapter->if_handle, NULL,
				&adapter->mc_cmd_mem);
		goto done;
	}

	be_cmd_multicast_set(adapter, adapter->if_handle, netdev,
		&adapter->mc_cmd_mem);
done:
	return;
}

static int be_set_vf_mac(struct net_device *netdev, int vf, u8 *mac)
{
	struct be_adapter *adapter = netdev_priv(netdev);
	int status;

	if (!adapter->sriov_enabled)
		return -EPERM;

	if (!is_valid_ether_addr(mac) || (vf >= num_vfs))
		return -EINVAL;

	if (adapter->vf_cfg[vf].vf_pmac_id != BE_INVALID_PMAC_ID)
		status = be_cmd_pmac_del(adapter,
					adapter->vf_cfg[vf].vf_if_handle,
					adapter->vf_cfg[vf].vf_pmac_id, vf + 1);

	status = be_cmd_pmac_add(adapter, mac,
				adapter->vf_cfg[vf].vf_if_handle,
				&adapter->vf_cfg[vf].vf_pmac_id, vf + 1);

	if (status)
		dev_err(&adapter->pdev->dev, "MAC %pM set on VF %d Failed\n",
				mac, vf);
	else
		memcpy(adapter->vf_cfg[vf].vf_mac_addr, mac, ETH_ALEN);

	return status;
}

static int be_get_vf_config(struct net_device *netdev, int vf,
			struct ifla_vf_info *vi)
{
	struct be_adapter *adapter = netdev_priv(netdev);

	if (!adapter->sriov_enabled)
		return -EPERM;

	if (vf >= num_vfs)
		return -EINVAL;

	vi->vf = vf;
	vi->tx_rate = adapter->vf_cfg[vf].vf_tx_rate;
	vi->vlan = adapter->vf_cfg[vf].vf_vlan_tag;
	vi->qos = 0;
	memcpy(&vi->mac, adapter->vf_cfg[vf].vf_mac_addr, ETH_ALEN);

	return 0;
}

static int be_set_vf_vlan(struct net_device *netdev,
			int vf, u16 vlan, u8 qos)
{
	struct be_adapter *adapter = netdev_priv(netdev);
	int status = 0;

	if (!adapter->sriov_enabled)
		return -EPERM;

	if ((vf >= num_vfs) || (vlan > 4095))
		return -EINVAL;

	if (vlan) {
		adapter->vf_cfg[vf].vf_vlan_tag = vlan;
		adapter->vlans_added++;
	} else {
		adapter->vf_cfg[vf].vf_vlan_tag = 0;
		adapter->vlans_added--;
	}

	status = be_vid_config(adapter, true, vf);

	if (status)
		dev_info(&adapter->pdev->dev,
				"VLAN %d config on VF %d failed\n", vlan, vf);
	return status;
}

static int be_set_vf_tx_rate(struct net_device *netdev,
			int vf, int rate)
{
	struct be_adapter *adapter = netdev_priv(netdev);
	int status = 0;

	if (!adapter->sriov_enabled)
		return -EPERM;

	if ((vf >= num_vfs) || (rate < 0))
		return -EINVAL;

	if (rate > 10000)
		rate = 10000;

	adapter->vf_cfg[vf].vf_tx_rate = rate;
	status = be_cmd_set_qos(adapter, rate / 10, vf + 1);

	if (status)
		dev_info(&adapter->pdev->dev,
				"tx rate %d on VF %d failed\n", rate, vf);
	return status;
}

static void be_rx_rate_update(struct be_rx_obj *rxo)
{
	struct be_rx_stats *stats = &rxo->stats;
	ulong now = jiffies;

	/* Wrapped around */
	if (time_before(now, stats->rx_jiffies)) {
		stats->rx_jiffies = now;
		return;
	}

	/* Update the rate once in two seconds */
	if ((now - stats->rx_jiffies) < 2 * HZ)
		return;

	stats->rx_rate = be_calc_rate(stats->rx_bytes - stats->rx_bytes_prev,
				now - stats->rx_jiffies);
	stats->rx_jiffies = now;
	stats->rx_bytes_prev = stats->rx_bytes;
}

static void be_rx_stats_update(struct be_rx_obj *rxo,
		struct be_rx_compl_info *rxcp)
{
	struct be_rx_stats *stats = &rxo->stats;

	stats->rx_compl++;
	stats->rx_frags += rxcp->num_rcvd;
	stats->rx_bytes += rxcp->pkt_size;
	stats->rx_pkts++;
	if (rxcp->pkt_type == BE_MULTICAST_PACKET)
		stats->rx_mcast_pkts++;
	if (rxcp->err)
		stats->rxcp_err++;
}

static inline bool csum_passed(struct be_rx_compl_info *rxcp)
{
	/* L4 checksum is not reliable for non TCP/UDP packets.
	 * Also ignore ipcksm for ipv6 pkts */
	return (rxcp->tcpf || rxcp->udpf) && rxcp->l4_csum &&
				(rxcp->ip_csum || rxcp->ipv6);
}

static struct be_rx_page_info *
get_rx_page_info(struct be_adapter *adapter,
		struct be_rx_obj *rxo,
		u16 frag_idx)
{
	struct be_rx_page_info *rx_page_info;
	struct be_queue_info *rxq = &rxo->q;

	rx_page_info = &rxo->page_info_tbl[frag_idx];
	BUG_ON(!rx_page_info->page);

	if (rx_page_info->last_page_user) {
		dma_unmap_page(&adapter->pdev->dev,
			       dma_unmap_addr(rx_page_info, bus),
			       adapter->big_page_size, DMA_FROM_DEVICE);
		rx_page_info->last_page_user = false;
	}

	atomic_dec(&rxq->used);
	return rx_page_info;
}

/* Throwaway the data in the Rx completion */
static void be_rx_compl_discard(struct be_adapter *adapter,
		struct be_rx_obj *rxo,
		struct be_rx_compl_info *rxcp)
{
	struct be_queue_info *rxq = &rxo->q;
	struct be_rx_page_info *page_info;
	u16 i, num_rcvd = rxcp->num_rcvd;

	for (i = 0; i < num_rcvd; i++) {
		page_info = get_rx_page_info(adapter, rxo, rxcp->rxq_idx);
		put_page(page_info->page);
		memset(page_info, 0, sizeof(*page_info));
		index_inc(&rxcp->rxq_idx, rxq->len);
	}
}

/*
 * skb_fill_rx_data forms a complete skb for an ether frame
 * indicated by rxcp.
 */
static void skb_fill_rx_data(struct be_adapter *adapter, struct be_rx_obj *rxo,
			struct sk_buff *skb, struct be_rx_compl_info *rxcp)
{
	struct be_queue_info *rxq = &rxo->q;
	struct be_rx_page_info *page_info;
	u16 i, j;
	u16 hdr_len, curr_frag_len, remaining;
	u8 *start;

	page_info = get_rx_page_info(adapter, rxo, rxcp->rxq_idx);
	start = page_address(page_info->page) + page_info->page_offset;
	prefetch(start);

	/* Copy data in the first descriptor of this completion */
	curr_frag_len = min(rxcp->pkt_size, rx_frag_size);

	/* Copy the header portion into skb_data */
	hdr_len = min(BE_HDR_LEN, curr_frag_len);
	memcpy(skb->data, start, hdr_len);
	skb->len = curr_frag_len;
	if (curr_frag_len <= BE_HDR_LEN) { /* tiny packet */
		/* Complete packet has now been moved to data */
		put_page(page_info->page);
		skb->data_len = 0;
		skb->tail += curr_frag_len;
	} else {
		skb_shinfo(skb)->nr_frags = 1;
		skb_shinfo(skb)->frags[0].page = page_info->page;
		skb_shinfo(skb)->frags[0].page_offset =
					page_info->page_offset + hdr_len;
		skb_shinfo(skb)->frags[0].size = curr_frag_len - hdr_len;
		skb->data_len = curr_frag_len - hdr_len;
		skb->tail += hdr_len;
	}
	page_info->page = NULL;

	if (rxcp->pkt_size <= rx_frag_size) {
		BUG_ON(rxcp->num_rcvd != 1);
		return;
	}

	/* More frags present for this completion */
	index_inc(&rxcp->rxq_idx, rxq->len);
	remaining = rxcp->pkt_size - curr_frag_len;
	for (i = 1, j = 0; i < rxcp->num_rcvd; i++) {
		page_info = get_rx_page_info(adapter, rxo, rxcp->rxq_idx);
		curr_frag_len = min(remaining, rx_frag_size);

		/* Coalesce all frags from the same physical page in one slot */
		if (page_info->page_offset == 0) {
			/* Fresh page */
			j++;
			skb_shinfo(skb)->frags[j].page = page_info->page;
			skb_shinfo(skb)->frags[j].page_offset =
							page_info->page_offset;
			skb_shinfo(skb)->frags[j].size = 0;
			skb_shinfo(skb)->nr_frags++;
		} else {
			put_page(page_info->page);
		}

		skb_shinfo(skb)->frags[j].size += curr_frag_len;
		skb->len += curr_frag_len;
		skb->data_len += curr_frag_len;

		remaining -= curr_frag_len;
		index_inc(&rxcp->rxq_idx, rxq->len);
		page_info->page = NULL;
	}
	BUG_ON(j > MAX_SKB_FRAGS);
}

/* Process the RX completion indicated by rxcp when GRO is disabled */
static void be_rx_compl_process(struct be_adapter *adapter,
			struct be_rx_obj *rxo,
			struct be_rx_compl_info *rxcp)
{
	struct net_device *netdev = adapter->netdev;
	struct sk_buff *skb;

	skb = netdev_alloc_skb_ip_align(netdev, BE_HDR_LEN);
	if (unlikely(!skb)) {
		if (net_ratelimit())
			dev_warn(&adapter->pdev->dev, "skb alloc failed\n");
		be_rx_compl_discard(adapter, rxo, rxcp);
		return;
	}

	skb_fill_rx_data(adapter, rxo, skb, rxcp);

	if (likely((netdev->features & NETIF_F_RXCSUM) && csum_passed(rxcp)))
		skb->ip_summed = CHECKSUM_UNNECESSARY;
	else
		skb_checksum_none_assert(skb);

	skb->truesize = skb->len + sizeof(struct sk_buff);
	skb->protocol = eth_type_trans(skb, netdev);
	if (adapter->netdev->features & NETIF_F_RXHASH)
		skb->rxhash = rxcp->rss_hash;


	if (unlikely(rxcp->vlanf)) {
		if (!adapter->vlan_grp || adapter->vlans_added == 0) {
			kfree_skb(skb);
			return;
		}
		vlan_hwaccel_receive_skb(skb, adapter->vlan_grp,
					rxcp->vlan_tag);
	} else {
		netif_receive_skb(skb);
	}
}

/* Process the RX completion indicated by rxcp when GRO is enabled */
static void be_rx_compl_process_gro(struct be_adapter *adapter,
		struct be_rx_obj *rxo,
		struct be_rx_compl_info *rxcp)
{
	struct be_rx_page_info *page_info;
	struct sk_buff *skb = NULL;
	struct be_queue_info *rxq = &rxo->q;
	struct be_eq_obj *eq_obj =  &rxo->rx_eq;
	u16 remaining, curr_frag_len;
	u16 i, j;

	skb = napi_get_frags(&eq_obj->napi);
	if (!skb) {
		be_rx_compl_discard(adapter, rxo, rxcp);
		return;
	}

	remaining = rxcp->pkt_size;
	for (i = 0, j = -1; i < rxcp->num_rcvd; i++) {
		page_info = get_rx_page_info(adapter, rxo, rxcp->rxq_idx);

		curr_frag_len = min(remaining, rx_frag_size);

		/* Coalesce all frags from the same physical page in one slot */
		if (i == 0 || page_info->page_offset == 0) {
			/* First frag or Fresh page */
			j++;
			skb_shinfo(skb)->frags[j].page = page_info->page;
			skb_shinfo(skb)->frags[j].page_offset =
							page_info->page_offset;
			skb_shinfo(skb)->frags[j].size = 0;
		} else {
			put_page(page_info->page);
		}
		skb_shinfo(skb)->frags[j].size += curr_frag_len;

		remaining -= curr_frag_len;
		index_inc(&rxcp->rxq_idx, rxq->len);
		memset(page_info, 0, sizeof(*page_info));
	}
	BUG_ON(j > MAX_SKB_FRAGS);

	skb_shinfo(skb)->nr_frags = j + 1;
	skb->len = rxcp->pkt_size;
	skb->data_len = rxcp->pkt_size;
	skb->truesize += rxcp->pkt_size;
	skb->ip_summed = CHECKSUM_UNNECESSARY;
	if (adapter->netdev->features & NETIF_F_RXHASH)
		skb->rxhash = rxcp->rss_hash;

	if (likely(!rxcp->vlanf))
		napi_gro_frags(&eq_obj->napi);
	else
		vlan_gro_frags(&eq_obj->napi, adapter->vlan_grp,
				rxcp->vlan_tag);
}

static void be_parse_rx_compl_v1(struct be_adapter *adapter,
				struct be_eth_rx_compl *compl,
				struct be_rx_compl_info *rxcp)
{
	rxcp->pkt_size =
		AMAP_GET_BITS(struct amap_eth_rx_compl_v1, pktsize, compl);
	rxcp->vlanf = AMAP_GET_BITS(struct amap_eth_rx_compl_v1, vtp, compl);
	rxcp->err = AMAP_GET_BITS(struct amap_eth_rx_compl_v1, err, compl);
	rxcp->tcpf = AMAP_GET_BITS(struct amap_eth_rx_compl_v1, tcpf, compl);
	rxcp->udpf = AMAP_GET_BITS(struct amap_eth_rx_compl_v1, udpf, compl);
	rxcp->ip_csum =
		AMAP_GET_BITS(struct amap_eth_rx_compl_v1, ipcksm, compl);
	rxcp->l4_csum =
		AMAP_GET_BITS(struct amap_eth_rx_compl_v1, l4_cksm, compl);
	rxcp->ipv6 =
		AMAP_GET_BITS(struct amap_eth_rx_compl_v1, ip_version, compl);
	rxcp->rxq_idx =
		AMAP_GET_BITS(struct amap_eth_rx_compl_v1, fragndx, compl);
	rxcp->num_rcvd =
		AMAP_GET_BITS(struct amap_eth_rx_compl_v1, numfrags, compl);
	rxcp->pkt_type =
		AMAP_GET_BITS(struct amap_eth_rx_compl_v1, cast_enc, compl);
	rxcp->rss_hash =
		AMAP_GET_BITS(struct amap_eth_rx_compl_v1, rsshash, rxcp);
	if (rxcp->vlanf) {
		rxcp->vtm = AMAP_GET_BITS(struct amap_eth_rx_compl_v1, vtm,
					  compl);
		rxcp->vlan_tag = AMAP_GET_BITS(struct amap_eth_rx_compl_v1, vlan_tag,
					       compl);
	}
}

static void be_parse_rx_compl_v0(struct be_adapter *adapter,
				struct be_eth_rx_compl *compl,
				struct be_rx_compl_info *rxcp)
{
	rxcp->pkt_size =
		AMAP_GET_BITS(struct amap_eth_rx_compl_v0, pktsize, compl);
	rxcp->vlanf = AMAP_GET_BITS(struct amap_eth_rx_compl_v0, vtp, compl);
	rxcp->err = AMAP_GET_BITS(struct amap_eth_rx_compl_v0, err, compl);
	rxcp->tcpf = AMAP_GET_BITS(struct amap_eth_rx_compl_v0, tcpf, compl);
	rxcp->udpf = AMAP_GET_BITS(struct amap_eth_rx_compl_v0, udpf, compl);
	rxcp->ip_csum =
		AMAP_GET_BITS(struct amap_eth_rx_compl_v0, ipcksm, compl);
	rxcp->l4_csum =
		AMAP_GET_BITS(struct amap_eth_rx_compl_v0, l4_cksm, compl);
	rxcp->ipv6 =
		AMAP_GET_BITS(struct amap_eth_rx_compl_v0, ip_version, compl);
	rxcp->rxq_idx =
		AMAP_GET_BITS(struct amap_eth_rx_compl_v0, fragndx, compl);
	rxcp->num_rcvd =
		AMAP_GET_BITS(struct amap_eth_rx_compl_v0, numfrags, compl);
	rxcp->pkt_type =
		AMAP_GET_BITS(struct amap_eth_rx_compl_v0, cast_enc, compl);
	rxcp->rss_hash =
		AMAP_GET_BITS(struct amap_eth_rx_compl_v0, rsshash, rxcp);
	if (rxcp->vlanf) {
		rxcp->vtm = AMAP_GET_BITS(struct amap_eth_rx_compl_v0, vtm,
					  compl);
		rxcp->vlan_tag = AMAP_GET_BITS(struct amap_eth_rx_compl_v0, vlan_tag,
					       compl);
	}
}

static struct be_rx_compl_info *be_rx_compl_get(struct be_rx_obj *rxo)
{
	struct be_eth_rx_compl *compl = queue_tail_node(&rxo->cq);
	struct be_rx_compl_info *rxcp = &rxo->rxcp;
	struct be_adapter *adapter = rxo->adapter;

	/* For checking the valid bit it is Ok to use either definition as the
	 * valid bit is at the same position in both v0 and v1 Rx compl */
	if (compl->dw[offsetof(struct amap_eth_rx_compl_v1, valid) / 32] == 0)
		return NULL;

	rmb();
	be_dws_le_to_cpu(compl, sizeof(*compl));

	if (adapter->be3_native)
		be_parse_rx_compl_v1(adapter, compl, rxcp);
	else
		be_parse_rx_compl_v0(adapter, compl, rxcp);

	if (rxcp->vlanf) {
		/* vlanf could be wrongly set in some cards.
		 * ignore if vtm is not set */
		if ((adapter->function_mode & 0x400) && !rxcp->vtm)
			rxcp->vlanf = 0;

		if (!lancer_chip(adapter))
			rxcp->vlan_tag = swab16(rxcp->vlan_tag);

		if (((adapter->pvid & VLAN_VID_MASK) ==
		     (rxcp->vlan_tag & VLAN_VID_MASK)) &&
		    !adapter->vlan_tag[rxcp->vlan_tag])
			rxcp->vlanf = 0;
	}

	/* As the compl has been parsed, reset it; we wont touch it again */
	compl->dw[offsetof(struct amap_eth_rx_compl_v1, valid) / 32] = 0;

	queue_tail_inc(&rxo->cq);
	return rxcp;
}

static inline struct page *be_alloc_pages(u32 size, gfp_t gfp)
{
	u32 order = get_order(size);

	if (order > 0)
		gfp |= __GFP_COMP;
	return  alloc_pages(gfp, order);
}

/*
 * Allocate a page, split it to fragments of size rx_frag_size and post as
 * receive buffers to BE
 */
static void be_post_rx_frags(struct be_rx_obj *rxo, gfp_t gfp)
{
	struct be_adapter *adapter = rxo->adapter;
	struct be_rx_page_info *page_info_tbl = rxo->page_info_tbl;
	struct be_rx_page_info *page_info = NULL, *prev_page_info = NULL;
	struct be_queue_info *rxq = &rxo->q;
	struct page *pagep = NULL;
	struct be_eth_rx_d *rxd;
	u64 page_dmaaddr = 0, frag_dmaaddr;
	u32 posted, page_offset = 0;

	page_info = &rxo->page_info_tbl[rxq->head];
	for (posted = 0; posted < MAX_RX_POST && !page_info->page; posted++) {
		if (!pagep) {
			pagep = be_alloc_pages(adapter->big_page_size, gfp);
			if (unlikely(!pagep)) {
				rxo->stats.rx_post_fail++;
				break;
			}
			page_dmaaddr = dma_map_page(&adapter->pdev->dev, pagep,
						    0, adapter->big_page_size,
						    DMA_FROM_DEVICE);
			page_info->page_offset = 0;
		} else {
			get_page(pagep);
			page_info->page_offset = page_offset + rx_frag_size;
		}
		page_offset = page_info->page_offset;
		page_info->page = pagep;
		dma_unmap_addr_set(page_info, bus, page_dmaaddr);
		frag_dmaaddr = page_dmaaddr + page_info->page_offset;

		rxd = queue_head_node(rxq);
		rxd->fragpa_lo = cpu_to_le32(frag_dmaaddr & 0xFFFFFFFF);
		rxd->fragpa_hi = cpu_to_le32(upper_32_bits(frag_dmaaddr));

		/* Any space left in the current big page for another frag? */
		if ((page_offset + rx_frag_size + rx_frag_size) >
					adapter->big_page_size) {
			pagep = NULL;
			page_info->last_page_user = true;
		}

		prev_page_info = page_info;
		queue_head_inc(rxq);
		page_info = &page_info_tbl[rxq->head];
	}
	if (pagep)
		prev_page_info->last_page_user = true;

	if (posted) {
		atomic_add(posted, &rxq->used);
		be_rxq_notify(adapter, rxq->id, posted);
	} else if (atomic_read(&rxq->used) == 0) {
		/* Let be_worker replenish when memory is available */
		rxo->rx_post_starved = true;
	}
}

static struct be_eth_tx_compl *be_tx_compl_get(struct be_queue_info *tx_cq)
{
	struct be_eth_tx_compl *txcp = queue_tail_node(tx_cq);

	if (txcp->dw[offsetof(struct amap_eth_tx_compl, valid) / 32] == 0)
		return NULL;

	rmb();
	be_dws_le_to_cpu(txcp, sizeof(*txcp));

	txcp->dw[offsetof(struct amap_eth_tx_compl, valid) / 32] = 0;

	queue_tail_inc(tx_cq);
	return txcp;
}

static u16 be_tx_compl_process(struct be_adapter *adapter,
		struct be_tx_obj *txo, u16 last_index)
{
	struct be_queue_info *txq = &txo->q;
	struct be_eth_wrb *wrb;
	struct sk_buff **sent_skbs = txo->sent_skb_list;
	struct sk_buff *sent_skb;
	u16 cur_index, num_wrbs = 1; /* account for hdr wrb */
	bool unmap_skb_hdr = true;

	sent_skb = sent_skbs[txq->tail];
	BUG_ON(!sent_skb);
	sent_skbs[txq->tail] = NULL;

	/* skip header wrb */
	queue_tail_inc(txq);

	do {
		cur_index = txq->tail;
		wrb = queue_tail_node(txq);
		unmap_tx_frag(&adapter->pdev->dev, wrb,
			      (unmap_skb_hdr && skb_headlen(sent_skb)));
		unmap_skb_hdr = false;

		num_wrbs++;
		queue_tail_inc(txq);
	} while (cur_index != last_index);

	kfree_skb(sent_skb);
	return num_wrbs;
}

static inline struct be_eq_entry *event_get(struct be_eq_obj *eq_obj)
{
	struct be_eq_entry *eqe = queue_tail_node(&eq_obj->q);

	if (!eqe->evt)
		return NULL;

	rmb();
	eqe->evt = le32_to_cpu(eqe->evt);
	queue_tail_inc(&eq_obj->q);
	return eqe;
}

static int event_handle(struct be_adapter *adapter,
			struct be_eq_obj *eq_obj,
			bool rearm)
{
	struct be_eq_entry *eqe;
	u16 num = 0;

	while ((eqe = event_get(eq_obj)) != NULL) {
		eqe->evt = 0;
		num++;
	}

	/* Deal with any spurious interrupts that come
	 * without events
	 */
	if (!num)
		rearm = true;

	be_eq_notify(adapter, eq_obj->q.id, rearm, true, num);
	if (num)
		napi_schedule(&eq_obj->napi);

	return num;
}

/* Just read and notify events without processing them.
 * Used at the time of destroying event queues */
static void be_eq_clean(struct be_adapter *adapter,
			struct be_eq_obj *eq_obj)
{
	struct be_eq_entry *eqe;
	u16 num = 0;

	while ((eqe = event_get(eq_obj)) != NULL) {
		eqe->evt = 0;
		num++;
	}

	if (num)
		be_eq_notify(adapter, eq_obj->q.id, false, true, num);
}

static void be_rx_q_clean(struct be_adapter *adapter, struct be_rx_obj *rxo)
{
	struct be_rx_page_info *page_info;
	struct be_queue_info *rxq = &rxo->q;
	struct be_queue_info *rx_cq = &rxo->cq;
	struct be_rx_compl_info *rxcp;
	u16 tail;

	/* First cleanup pending rx completions */
	while ((rxcp = be_rx_compl_get(rxo)) != NULL) {
		be_rx_compl_discard(adapter, rxo, rxcp);
		be_cq_notify(adapter, rx_cq->id, false, 1);
	}

	/* Then free posted rx buffer that were not used */
	tail = (rxq->head + rxq->len - atomic_read(&rxq->used)) % rxq->len;
	for (; atomic_read(&rxq->used) > 0; index_inc(&tail, rxq->len)) {
		page_info = get_rx_page_info(adapter, rxo, tail);
		put_page(page_info->page);
		memset(page_info, 0, sizeof(*page_info));
	}
	BUG_ON(atomic_read(&rxq->used));
}

static void be_tx_compl_clean(struct be_adapter *adapter,
				struct be_tx_obj *txo)
{
	struct be_queue_info *tx_cq = &txo->cq;
	struct be_queue_info *txq = &txo->q;
	struct be_eth_tx_compl *txcp;
	u16 end_idx, cmpl = 0, timeo = 0, num_wrbs = 0;
	struct sk_buff **sent_skbs = txo->sent_skb_list;
	struct sk_buff *sent_skb;
	bool dummy_wrb;

	/* Wait for a max of 200ms for all the tx-completions to arrive. */
	do {
		while ((txcp = be_tx_compl_get(tx_cq))) {
			end_idx = AMAP_GET_BITS(struct amap_eth_tx_compl,
					wrb_index, txcp);
			num_wrbs += be_tx_compl_process(adapter, txo, end_idx);
			cmpl++;
		}
		if (cmpl) {
			be_cq_notify(adapter, tx_cq->id, false, cmpl);
			atomic_sub(num_wrbs, &txq->used);
			cmpl = 0;
			num_wrbs = 0;
		}

		if (atomic_read(&txq->used) == 0 || ++timeo > 200)
			break;

		mdelay(1);
	} while (true);

	if (atomic_read(&txq->used))
		dev_err(&adapter->pdev->dev, "%d pending tx-completions\n",
			atomic_read(&txq->used));

	/* free posted tx for which compls will never arrive */
	while (atomic_read(&txq->used)) {
		sent_skb = sent_skbs[txq->tail];
		end_idx = txq->tail;
		index_adv(&end_idx,
			wrb_cnt_for_skb(adapter, sent_skb, &dummy_wrb) - 1,
			txq->len);
		num_wrbs = be_tx_compl_process(adapter, txo, end_idx);
		atomic_sub(num_wrbs, &txq->used);
	}
}

static void be_mcc_queues_destroy(struct be_adapter *adapter)
{
	struct be_queue_info *q;

	q = &adapter->mcc_obj.q;
	if (q->created)
		be_cmd_q_destroy(adapter, q, QTYPE_MCCQ);
	be_queue_free(adapter, q);

	q = &adapter->mcc_obj.cq;
	if (q->created)
		be_cmd_q_destroy(adapter, q, QTYPE_CQ);
	be_queue_free(adapter, q);
}

/* Must be called only after TX qs are created as MCC shares TX EQ */
static int be_mcc_queues_create(struct be_adapter *adapter)
{
	struct be_queue_info *q, *cq;

	/* Alloc MCC compl queue */
	cq = &adapter->mcc_obj.cq;
	if (be_queue_alloc(adapter, cq, MCC_CQ_LEN,
			sizeof(struct be_mcc_compl)))
		goto err;

	/* Ask BE to create MCC compl queue; share TX's eq */
	if (be_cmd_cq_create(adapter, cq, &adapter->tx_eq.q, false, true, 0))
		goto mcc_cq_free;

	/* Alloc MCC queue */
	q = &adapter->mcc_obj.q;
	if (be_queue_alloc(adapter, q, MCC_Q_LEN, sizeof(struct be_mcc_wrb)))
		goto mcc_cq_destroy;

	/* Ask BE to create MCC queue */
	if (be_cmd_mccq_create(adapter, q, cq))
		goto mcc_q_free;

	return 0;

mcc_q_free:
	be_queue_free(adapter, q);
mcc_cq_destroy:
	be_cmd_q_destroy(adapter, cq, QTYPE_CQ);
mcc_cq_free:
	be_queue_free(adapter, cq);
err:
	return -1;
}

static void be_tx_queues_destroy(struct be_adapter *adapter)
{
	struct be_queue_info *q;
	struct be_tx_obj *txo;
	u8 i;

	for_all_tx_queues(adapter, txo, i) {
		q = &txo->q;
		if (q->created)
			be_cmd_q_destroy(adapter, q, QTYPE_TXQ);
		be_queue_free(adapter, q);

		q = &txo->cq;
		if (q->created)
			be_cmd_q_destroy(adapter, q, QTYPE_CQ);
		be_queue_free(adapter, q);
	}

	/* Clear any residual events */
	be_eq_clean(adapter, &adapter->tx_eq);

	q = &adapter->tx_eq.q;
	if (q->created)
		be_cmd_q_destroy(adapter, q, QTYPE_EQ);
	be_queue_free(adapter, q);
}

/* One TX event queue is shared by all TX compl qs */
static int be_tx_queues_create(struct be_adapter *adapter)
{
	struct be_queue_info *eq, *q, *cq;
	struct be_tx_obj *txo;
	u8 i;

	adapter->tx_eq.max_eqd = 0;
	adapter->tx_eq.min_eqd = 0;
	adapter->tx_eq.cur_eqd = 96;
	adapter->tx_eq.enable_aic = false;

	eq = &adapter->tx_eq.q;
	if (be_queue_alloc(adapter, eq, EVNT_Q_LEN,
		sizeof(struct be_eq_entry)))
		return -1;

	if (be_cmd_eq_create(adapter, eq, adapter->tx_eq.cur_eqd))
		goto err;
	adapter->tx_eq.eq_idx = adapter->eq_next_idx++;

	for_all_tx_queues(adapter, txo, i) {
		cq = &txo->cq;
		if (be_queue_alloc(adapter, cq, TX_CQ_LEN,
			sizeof(struct be_eth_tx_compl)))
			goto err;

		if (be_cmd_cq_create(adapter, cq, eq, false, false, 3))
			goto err;

		q = &txo->q;
		if (be_queue_alloc(adapter, q, TX_Q_LEN,
			sizeof(struct be_eth_wrb)))
			goto err;

		if (be_cmd_txq_create(adapter, q, cq))
			goto err;
	}
	return 0;

err:
	be_tx_queues_destroy(adapter);
	return -1;
}

static void be_rx_queues_destroy(struct be_adapter *adapter)
{
	struct be_queue_info *q;
	struct be_rx_obj *rxo;
	int i;

	for_all_rx_queues(adapter, rxo, i) {
		q = &rxo->q;
		if (q->created) {
			be_cmd_q_destroy(adapter, q, QTYPE_RXQ);
			/* After the rxq is invalidated, wait for a grace time
			 * of 1ms for all dma to end and the flush compl to
			 * arrive
			 */
			mdelay(1);
			be_rx_q_clean(adapter, rxo);
		}
		be_queue_free(adapter, q);

		q = &rxo->cq;
		if (q->created)
			be_cmd_q_destroy(adapter, q, QTYPE_CQ);
		be_queue_free(adapter, q);

		/* Clear any residual events */
		q = &rxo->rx_eq.q;
		if (q->created) {
			be_eq_clean(adapter, &rxo->rx_eq);
			be_cmd_q_destroy(adapter, q, QTYPE_EQ);
		}
		be_queue_free(adapter, q);
	}
}

static u32 be_num_rxqs_want(struct be_adapter *adapter)
{
	if ((adapter->function_caps & BE_FUNCTION_CAPS_RSS) &&
		!adapter->sriov_enabled && !(adapter->function_mode & 0x400)) {
		return 1 + MAX_RSS_QS; /* one default non-RSS queue */
	} else {
		dev_warn(&adapter->pdev->dev,
			"No support for multiple RX queues\n");
		return 1;
	}
}

static int be_rx_queues_create(struct be_adapter *adapter)
{
	struct be_queue_info *eq, *q, *cq;
	struct be_rx_obj *rxo;
	int rc, i;

	adapter->num_rx_qs = min(be_num_rxqs_want(adapter),
				msix_enabled(adapter) ?
					adapter->num_msix_vec - 1 : 1);
	if (adapter->num_rx_qs != MAX_RX_QS)
		dev_warn(&adapter->pdev->dev,
			"Can create only %d RX queues", adapter->num_rx_qs);

	adapter->big_page_size = (1 << get_order(rx_frag_size)) * PAGE_SIZE;
	for_all_rx_queues(adapter, rxo, i) {
		rxo->adapter = adapter;
		rxo->rx_eq.max_eqd = BE_MAX_EQD;
		rxo->rx_eq.enable_aic = true;

		/* EQ */
		eq = &rxo->rx_eq.q;
		rc = be_queue_alloc(adapter, eq, EVNT_Q_LEN,
					sizeof(struct be_eq_entry));
		if (rc)
			goto err;

		rc = be_cmd_eq_create(adapter, eq, rxo->rx_eq.cur_eqd);
		if (rc)
			goto err;

		rxo->rx_eq.eq_idx = adapter->eq_next_idx++;

		/* CQ */
		cq = &rxo->cq;
		rc = be_queue_alloc(adapter, cq, RX_CQ_LEN,
				sizeof(struct be_eth_rx_compl));
		if (rc)
			goto err;

		rc = be_cmd_cq_create(adapter, cq, eq, false, false, 3);
		if (rc)
			goto err;
		/* Rx Q */
		q = &rxo->q;
		rc = be_queue_alloc(adapter, q, RX_Q_LEN,
				sizeof(struct be_eth_rx_d));
		if (rc)
			goto err;

		rc = be_cmd_rxq_create(adapter, q, cq->id, rx_frag_size,
			BE_MAX_JUMBO_FRAME_SIZE, adapter->if_handle,
			(i > 0) ? 1 : 0/* rss enable */, &rxo->rss_id);
		if (rc)
			goto err;
	}

	if (be_multi_rxq(adapter)) {
		u8 rsstable[MAX_RSS_QS];

		for_all_rss_queues(adapter, rxo, i)
			rsstable[i] = rxo->rss_id;

		rc = be_cmd_rss_config(adapter, rsstable,
			adapter->num_rx_qs - 1);
		if (rc)
			goto err;
	}

	return 0;
err:
	be_rx_queues_destroy(adapter);
	return -1;
}

static bool event_peek(struct be_eq_obj *eq_obj)
{
	struct be_eq_entry *eqe = queue_tail_node(&eq_obj->q);
	if (!eqe->evt)
		return false;
	else
		return true;
}

static irqreturn_t be_intx(int irq, void *dev)
{
	struct be_adapter *adapter = dev;
	struct be_rx_obj *rxo;
	int isr, i, tx = 0 , rx = 0;

	if (lancer_chip(adapter)) {
		if (event_peek(&adapter->tx_eq))
			tx = event_handle(adapter, &adapter->tx_eq, false);
		for_all_rx_queues(adapter, rxo, i) {
			if (event_peek(&rxo->rx_eq))
				rx |= event_handle(adapter, &rxo->rx_eq, true);
		}

		if (!(tx || rx))
			return IRQ_NONE;

	} else {
		isr = ioread32(adapter->csr + CEV_ISR0_OFFSET +
			(adapter->tx_eq.q.id / 8) * CEV_ISR_SIZE);
		if (!isr)
			return IRQ_NONE;

		if ((1 << adapter->tx_eq.eq_idx & isr))
			event_handle(adapter, &adapter->tx_eq, false);

		for_all_rx_queues(adapter, rxo, i) {
			if ((1 << rxo->rx_eq.eq_idx & isr))
				event_handle(adapter, &rxo->rx_eq, true);
		}
	}

	return IRQ_HANDLED;
}

static irqreturn_t be_msix_rx(int irq, void *dev)
{
	struct be_rx_obj *rxo = dev;
	struct be_adapter *adapter = rxo->adapter;

	event_handle(adapter, &rxo->rx_eq, true);

	return IRQ_HANDLED;
}

static irqreturn_t be_msix_tx_mcc(int irq, void *dev)
{
	struct be_adapter *adapter = dev;

	event_handle(adapter, &adapter->tx_eq, false);

	return IRQ_HANDLED;
}

static inline bool do_gro(struct be_rx_compl_info *rxcp)
{
	return (rxcp->tcpf && !rxcp->err) ? true : false;
}

static int be_poll_rx(struct napi_struct *napi, int budget)
{
	struct be_eq_obj *rx_eq = container_of(napi, struct be_eq_obj, napi);
	struct be_rx_obj *rxo = container_of(rx_eq, struct be_rx_obj, rx_eq);
	struct be_adapter *adapter = rxo->adapter;
	struct be_queue_info *rx_cq = &rxo->cq;
	struct be_rx_compl_info *rxcp;
	u32 work_done;

	rxo->stats.rx_polls++;
	for (work_done = 0; work_done < budget; work_done++) {
		rxcp = be_rx_compl_get(rxo);
		if (!rxcp)
			break;

		/* Ignore flush completions */
		if (rxcp->num_rcvd && rxcp->pkt_size) {
			if (do_gro(rxcp))
				be_rx_compl_process_gro(adapter, rxo, rxcp);
			else
				be_rx_compl_process(adapter, rxo, rxcp);
		} else if (rxcp->pkt_size == 0) {
			be_rx_compl_discard(adapter, rxo, rxcp);
		}

		be_rx_stats_update(rxo, rxcp);
	}

	/* Refill the queue */
	if (atomic_read(&rxo->q.used) < RX_FRAGS_REFILL_WM)
		be_post_rx_frags(rxo, GFP_ATOMIC);

	/* All consumed */
	if (work_done < budget) {
		napi_complete(napi);
		be_cq_notify(adapter, rx_cq->id, true, work_done);
	} else {
		/* More to be consumed; continue with interrupts disabled */
		be_cq_notify(adapter, rx_cq->id, false, work_done);
	}
	return work_done;
}

/* As TX and MCC share the same EQ check for both TX and MCC completions.
 * For TX/MCC we don't honour budget; consume everything
 */
static int be_poll_tx_mcc(struct napi_struct *napi, int budget)
{
	struct be_eq_obj *tx_eq = container_of(napi, struct be_eq_obj, napi);
	struct be_adapter *adapter =
		container_of(tx_eq, struct be_adapter, tx_eq);
	struct be_tx_obj *txo;
	struct be_eth_tx_compl *txcp;
	int tx_compl, mcc_compl, status = 0;
	u8 i;
	u16 num_wrbs;

	for_all_tx_queues(adapter, txo, i) {
		tx_compl = 0;
		num_wrbs = 0;
		while ((txcp = be_tx_compl_get(&txo->cq))) {
			num_wrbs += be_tx_compl_process(adapter, txo,
				AMAP_GET_BITS(struct amap_eth_tx_compl,
					wrb_index, txcp));
			tx_compl++;
		}
		if (tx_compl) {
			be_cq_notify(adapter, txo->cq.id, true, tx_compl);

			atomic_sub(num_wrbs, &txo->q.used);

			/* As Tx wrbs have been freed up, wake up netdev queue
			 * if it was stopped due to lack of tx wrbs.  */
			if (__netif_subqueue_stopped(adapter->netdev, i) &&
				atomic_read(&txo->q.used) < txo->q.len / 2) {
				netif_wake_subqueue(adapter->netdev, i);
			}

			adapter->drv_stats.be_tx_events++;
			txo->stats.be_tx_compl += tx_compl;
		}
	}

	mcc_compl = be_process_mcc(adapter, &status);

	if (mcc_compl) {
		struct be_mcc_obj *mcc_obj = &adapter->mcc_obj;
		be_cq_notify(adapter, mcc_obj->cq.id, true, mcc_compl);
	}

	napi_complete(napi);

	be_eq_notify(adapter, tx_eq->q.id, true, false, 0);
	return 1;
}

void be_detect_dump_ue(struct be_adapter *adapter)
{
	u32 ue_status_lo, ue_status_hi, ue_status_lo_mask, ue_status_hi_mask;
	u32 i;

	pci_read_config_dword(adapter->pdev,
				PCICFG_UE_STATUS_LOW, &ue_status_lo);
	pci_read_config_dword(adapter->pdev,
				PCICFG_UE_STATUS_HIGH, &ue_status_hi);
	pci_read_config_dword(adapter->pdev,
				PCICFG_UE_STATUS_LOW_MASK, &ue_status_lo_mask);
	pci_read_config_dword(adapter->pdev,
				PCICFG_UE_STATUS_HI_MASK, &ue_status_hi_mask);

	ue_status_lo = (ue_status_lo & (~ue_status_lo_mask));
	ue_status_hi = (ue_status_hi & (~ue_status_hi_mask));

	if (ue_status_lo || ue_status_hi) {
		adapter->ue_detected = true;
		adapter->eeh_err = true;
		dev_err(&adapter->pdev->dev, "UE Detected!!\n");
	}

	if (ue_status_lo) {
		for (i = 0; ue_status_lo; ue_status_lo >>= 1, i++) {
			if (ue_status_lo & 1)
				dev_err(&adapter->pdev->dev,
				"UE: %s bit set\n", ue_status_low_desc[i]);
		}
	}
	if (ue_status_hi) {
		for (i = 0; ue_status_hi; ue_status_hi >>= 1, i++) {
			if (ue_status_hi & 1)
				dev_err(&adapter->pdev->dev,
				"UE: %s bit set\n", ue_status_hi_desc[i]);
		}
	}

}

static void be_worker(struct work_struct *work)
{
	struct be_adapter *adapter =
		container_of(work, struct be_adapter, work.work);
	struct be_rx_obj *rxo;
	struct be_tx_obj *txo;
	int i;

	if (!adapter->ue_detected && !lancer_chip(adapter))
		be_detect_dump_ue(adapter);

	/* when interrupts are not yet enabled, just reap any pending
	* mcc completions */
	if (!netif_running(adapter->netdev)) {
		int mcc_compl, status = 0;

		mcc_compl = be_process_mcc(adapter, &status);

		if (mcc_compl) {
			struct be_mcc_obj *mcc_obj = &adapter->mcc_obj;
			be_cq_notify(adapter, mcc_obj->cq.id, false, mcc_compl);
		}

		goto reschedule;
	}

	if (!adapter->stats_cmd_sent) {
		if (lancer_chip(adapter))
			lancer_cmd_get_pport_stats(adapter,
						&adapter->stats_cmd);
		else
			be_cmd_get_stats(adapter, &adapter->stats_cmd);
	}

	for_all_tx_queues(adapter, txo, i)
		be_tx_rate_update(txo);

	for_all_rx_queues(adapter, rxo, i) {
		be_rx_rate_update(rxo);
		be_rx_eqd_update(adapter, rxo);

		if (rxo->rx_post_starved) {
			rxo->rx_post_starved = false;
			be_post_rx_frags(rxo, GFP_KERNEL);
		}
	}

reschedule:
	adapter->work_counter++;
	schedule_delayed_work(&adapter->work, msecs_to_jiffies(1000));
}

static void be_msix_disable(struct be_adapter *adapter)
{
	if (msix_enabled(adapter)) {
		pci_disable_msix(adapter->pdev);
		adapter->num_msix_vec = 0;
	}
}

static void be_msix_enable(struct be_adapter *adapter)
{
#define BE_MIN_MSIX_VECTORS	(1 + 1) /* Rx + Tx */
	int i, status, num_vec;

	num_vec = be_num_rxqs_want(adapter) + 1;

	for (i = 0; i < num_vec; i++)
		adapter->msix_entries[i].entry = i;

	status = pci_enable_msix(adapter->pdev, adapter->msix_entries, num_vec);
	if (status == 0) {
		goto done;
	} else if (status >= BE_MIN_MSIX_VECTORS) {
		num_vec = status;
		if (pci_enable_msix(adapter->pdev, adapter->msix_entries,
				num_vec) == 0)
			goto done;
	}
	return;
done:
	adapter->num_msix_vec = num_vec;
	return;
}

static void be_sriov_enable(struct be_adapter *adapter)
{
	be_check_sriov_fn_type(adapter);
#ifdef CONFIG_PCI_IOV
	if (be_physfn(adapter) && num_vfs) {
		int status, pos;
		u16 nvfs;

		pos = pci_find_ext_capability(adapter->pdev,
						PCI_EXT_CAP_ID_SRIOV);
		pci_read_config_word(adapter->pdev,
					pos + PCI_SRIOV_TOTAL_VF, &nvfs);

		if (num_vfs > nvfs) {
			dev_info(&adapter->pdev->dev,
					"Device supports %d VFs and not %d\n",
					nvfs, num_vfs);
			num_vfs = nvfs;
		}

		status = pci_enable_sriov(adapter->pdev, num_vfs);
		adapter->sriov_enabled = status ? false : true;
	}
#endif
}

static void be_sriov_disable(struct be_adapter *adapter)
{
#ifdef CONFIG_PCI_IOV
	if (adapter->sriov_enabled) {
		pci_disable_sriov(adapter->pdev);
		adapter->sriov_enabled = false;
	}
#endif
}

static inline int be_msix_vec_get(struct be_adapter *adapter,
					struct be_eq_obj *eq_obj)
{
	return adapter->msix_entries[eq_obj->eq_idx].vector;
}

static int be_request_irq(struct be_adapter *adapter,
		struct be_eq_obj *eq_obj,
		void *handler, char *desc, void *context)
{
	struct net_device *netdev = adapter->netdev;
	int vec;

	sprintf(eq_obj->desc, "%s-%s", netdev->name, desc);
	vec = be_msix_vec_get(adapter, eq_obj);
	return request_irq(vec, handler, 0, eq_obj->desc, context);
}

static void be_free_irq(struct be_adapter *adapter, struct be_eq_obj *eq_obj,
			void *context)
{
	int vec = be_msix_vec_get(adapter, eq_obj);
	free_irq(vec, context);
}

static int be_msix_register(struct be_adapter *adapter)
{
	struct be_rx_obj *rxo;
	int status, i;
	char qname[10];

	status = be_request_irq(adapter, &adapter->tx_eq, be_msix_tx_mcc, "tx",
				adapter);
	if (status)
		goto err;

	for_all_rx_queues(adapter, rxo, i) {
		sprintf(qname, "rxq%d", i);
		status = be_request_irq(adapter, &rxo->rx_eq, be_msix_rx,
				qname, rxo);
		if (status)
			goto err_msix;
	}

	return 0;

err_msix:
	be_free_irq(adapter, &adapter->tx_eq, adapter);

	for (i--, rxo = &adapter->rx_obj[i]; i >= 0; i--, rxo--)
		be_free_irq(adapter, &rxo->rx_eq, rxo);

err:
	dev_warn(&adapter->pdev->dev,
		"MSIX Request IRQ failed - err %d\n", status);
	be_msix_disable(adapter);
	return status;
}

static int be_irq_register(struct be_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	int status;

	if (msix_enabled(adapter)) {
		status = be_msix_register(adapter);
		if (status == 0)
			goto done;
		/* INTx is not supported for VF */
		if (!be_physfn(adapter))
			return status;
	}

	/* INTx */
	netdev->irq = adapter->pdev->irq;
	status = request_irq(netdev->irq, be_intx, IRQF_SHARED, netdev->name,
			adapter);
	if (status) {
		dev_err(&adapter->pdev->dev,
			"INTx request IRQ failed - err %d\n", status);
		return status;
	}
done:
	adapter->isr_registered = true;
	return 0;
}

static void be_irq_unregister(struct be_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	struct be_rx_obj *rxo;
	int i;

	if (!adapter->isr_registered)
		return;

	/* INTx */
	if (!msix_enabled(adapter)) {
		free_irq(netdev->irq, adapter);
		goto done;
	}

	/* MSIx */
	be_free_irq(adapter, &adapter->tx_eq, adapter);

	for_all_rx_queues(adapter, rxo, i)
		be_free_irq(adapter, &rxo->rx_eq, rxo);

done:
	adapter->isr_registered = false;
}

static int be_close(struct net_device *netdev)
{
	struct be_adapter *adapter = netdev_priv(netdev);
	struct be_rx_obj *rxo;
	struct be_tx_obj *txo;
	struct be_eq_obj *tx_eq = &adapter->tx_eq;
	int vec, i;

	be_async_mcc_disable(adapter);

	netif_carrier_off(netdev);
	adapter->link_up = false;

	if (!lancer_chip(adapter))
		be_intr_set(adapter, false);

	for_all_rx_queues(adapter, rxo, i)
		napi_disable(&rxo->rx_eq.napi);

	napi_disable(&tx_eq->napi);

	if (lancer_chip(adapter)) {
		be_cq_notify(adapter, adapter->mcc_obj.cq.id, false, 0);
		for_all_rx_queues(adapter, rxo, i)
			 be_cq_notify(adapter, rxo->cq.id, false, 0);
		for_all_tx_queues(adapter, txo, i)
			 be_cq_notify(adapter, txo->cq.id, false, 0);
	}

	if (msix_enabled(adapter)) {
		vec = be_msix_vec_get(adapter, tx_eq);
		synchronize_irq(vec);

		for_all_rx_queues(adapter, rxo, i) {
			vec = be_msix_vec_get(adapter, &rxo->rx_eq);
			synchronize_irq(vec);
		}
	} else {
		synchronize_irq(netdev->irq);
	}
	be_irq_unregister(adapter);

	/* Wait for all pending tx completions to arrive so that
	 * all tx skbs are freed.
	 */
	for_all_tx_queues(adapter, txo, i)
		be_tx_compl_clean(adapter, txo);

	return 0;
}

static int be_open(struct net_device *netdev)
{
	struct be_adapter *adapter = netdev_priv(netdev);
	struct be_eq_obj *tx_eq = &adapter->tx_eq;
	struct be_rx_obj *rxo;
	bool link_up;
	int status, i;
	u8 mac_speed;
	u16 link_speed;

	for_all_rx_queues(adapter, rxo, i) {
		be_post_rx_frags(rxo, GFP_KERNEL);
		napi_enable(&rxo->rx_eq.napi);
	}
	napi_enable(&tx_eq->napi);

	be_irq_register(adapter);

	if (!lancer_chip(adapter))
		be_intr_set(adapter, true);

	/* The evt queues are created in unarmed state; arm them */
	for_all_rx_queues(adapter, rxo, i) {
		be_eq_notify(adapter, rxo->rx_eq.q.id, true, false, 0);
		be_cq_notify(adapter, rxo->cq.id, true, 0);
	}
	be_eq_notify(adapter, tx_eq->q.id, true, false, 0);

	/* Now that interrupts are on we can process async mcc */
	be_async_mcc_enable(adapter);

	status = be_cmd_link_status_query(adapter, &link_up, &mac_speed,
			&link_speed, 0);
	if (status)
		goto err;
	be_link_status_update(adapter, link_up);

	if (be_physfn(adapter)) {
		status = be_vid_config(adapter, false, 0);
		if (status)
			goto err;

		status = be_cmd_set_flow_control(adapter,
				adapter->tx_fc, adapter->rx_fc);
		if (status)
			goto err;
	}

	return 0;
err:
	be_close(adapter->netdev);
	return -EIO;
}

static int be_setup_wol(struct be_adapter *adapter, bool enable)
{
	struct be_dma_mem cmd;
	int status = 0;
	u8 mac[ETH_ALEN];

	memset(mac, 0, ETH_ALEN);

	cmd.size = sizeof(struct be_cmd_req_acpi_wol_magic_config);
	cmd.va = dma_alloc_coherent(&adapter->pdev->dev, cmd.size, &cmd.dma,
				    GFP_KERNEL);
	if (cmd.va == NULL)
		return -1;
	memset(cmd.va, 0, cmd.size);

	if (enable) {
		status = pci_write_config_dword(adapter->pdev,
			PCICFG_PM_CONTROL_OFFSET, PCICFG_PM_CONTROL_MASK);
		if (status) {
			dev_err(&adapter->pdev->dev,
				"Could not enable Wake-on-lan\n");
			dma_free_coherent(&adapter->pdev->dev, cmd.size, cmd.va,
					  cmd.dma);
			return status;
		}
		status = be_cmd_enable_magic_wol(adapter,
				adapter->netdev->dev_addr, &cmd);
		pci_enable_wake(adapter->pdev, PCI_D3hot, 1);
		pci_enable_wake(adapter->pdev, PCI_D3cold, 1);
	} else {
		status = be_cmd_enable_magic_wol(adapter, mac, &cmd);
		pci_enable_wake(adapter->pdev, PCI_D3hot, 0);
		pci_enable_wake(adapter->pdev, PCI_D3cold, 0);
	}

	dma_free_coherent(&adapter->pdev->dev, cmd.size, cmd.va, cmd.dma);
	return status;
}

/*
 * Generate a seed MAC address from the PF MAC Address using jhash.
 * MAC Address for VFs are assigned incrementally starting from the seed.
 * These addresses are programmed in the ASIC by the PF and the VF driver
 * queries for the MAC address during its probe.
 */
static inline int be_vf_eth_addr_config(struct be_adapter *adapter)
{
	u32 vf = 0;
	int status = 0;
	u8 mac[ETH_ALEN];

	be_vf_eth_addr_generate(adapter, mac);

	for (vf = 0; vf < num_vfs; vf++) {
		status = be_cmd_pmac_add(adapter, mac,
					adapter->vf_cfg[vf].vf_if_handle,
					&adapter->vf_cfg[vf].vf_pmac_id,
					vf + 1);
		if (status)
			dev_err(&adapter->pdev->dev,
				"Mac address add failed for VF %d\n", vf);
		else
			memcpy(adapter->vf_cfg[vf].vf_mac_addr, mac, ETH_ALEN);

		mac[5] += 1;
	}
	return status;
}

static inline void be_vf_eth_addr_rem(struct be_adapter *adapter)
{
	u32 vf;

	for (vf = 0; vf < num_vfs; vf++) {
		if (adapter->vf_cfg[vf].vf_pmac_id != BE_INVALID_PMAC_ID)
			be_cmd_pmac_del(adapter,
					adapter->vf_cfg[vf].vf_if_handle,
					adapter->vf_cfg[vf].vf_pmac_id, vf + 1);
	}
}

static int be_setup(struct be_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	u32 cap_flags, en_flags, vf = 0;
	int status;
	u8 mac[ETH_ALEN];

	cap_flags = en_flags = BE_IF_FLAGS_UNTAGGED |
				BE_IF_FLAGS_BROADCAST |
				BE_IF_FLAGS_MULTICAST;

	if (be_physfn(adapter)) {
		cap_flags |= BE_IF_FLAGS_MCAST_PROMISCUOUS |
				BE_IF_FLAGS_PROMISCUOUS |
				BE_IF_FLAGS_PASS_L3L4_ERRORS;
		en_flags |= BE_IF_FLAGS_PASS_L3L4_ERRORS;

		if (adapter->function_caps & BE_FUNCTION_CAPS_RSS) {
			cap_flags |= BE_IF_FLAGS_RSS;
			en_flags |= BE_IF_FLAGS_RSS;
		}
	}

	status = be_cmd_if_create(adapter, cap_flags, en_flags,
			netdev->dev_addr, false/* pmac_invalid */,
			&adapter->if_handle, &adapter->pmac_id, 0);
	if (status != 0)
		goto do_none;

	if (be_physfn(adapter)) {
		if (adapter->sriov_enabled) {
			while (vf < num_vfs) {
				cap_flags = en_flags = BE_IF_FLAGS_UNTAGGED |
							BE_IF_FLAGS_BROADCAST;
				status = be_cmd_if_create(adapter, cap_flags,
					en_flags, mac, true,
					&adapter->vf_cfg[vf].vf_if_handle,
					NULL, vf+1);
				if (status) {
					dev_err(&adapter->pdev->dev,
					"Interface Create failed for VF %d\n",
					vf);
					goto if_destroy;
				}
				adapter->vf_cfg[vf].vf_pmac_id =
							BE_INVALID_PMAC_ID;
				vf++;
			}
		}
	} else {
		status = be_cmd_mac_addr_query(adapter, mac,
			MAC_ADDRESS_TYPE_NETWORK, false, adapter->if_handle);
		if (!status) {
			memcpy(adapter->netdev->dev_addr, mac, ETH_ALEN);
			memcpy(adapter->netdev->perm_addr, mac, ETH_ALEN);
		}
	}

	status = be_tx_queues_create(adapter);
	if (status != 0)
		goto if_destroy;

	status = be_rx_queues_create(adapter);
	if (status != 0)
		goto tx_qs_destroy;

	status = be_mcc_queues_create(adapter);
	if (status != 0)
		goto rx_qs_destroy;

	adapter->link_speed = -1;

	return 0;

rx_qs_destroy:
	be_rx_queues_destroy(adapter);
tx_qs_destroy:
	be_tx_queues_destroy(adapter);
if_destroy:
	if (be_physfn(adapter) && adapter->sriov_enabled)
		for (vf = 0; vf < num_vfs; vf++)
			if (adapter->vf_cfg[vf].vf_if_handle)
				be_cmd_if_destroy(adapter,
					adapter->vf_cfg[vf].vf_if_handle,
					vf + 1);
	be_cmd_if_destroy(adapter, adapter->if_handle, 0);
do_none:
	return status;
}

static int be_clear(struct be_adapter *adapter)
{
	int vf;

	if (be_physfn(adapter) && adapter->sriov_enabled)
		be_vf_eth_addr_rem(adapter);

	be_mcc_queues_destroy(adapter);
	be_rx_queues_destroy(adapter);
	be_tx_queues_destroy(adapter);
	adapter->eq_next_idx = 0;

	if (be_physfn(adapter) && adapter->sriov_enabled)
		for (vf = 0; vf < num_vfs; vf++)
			if (adapter->vf_cfg[vf].vf_if_handle)
				be_cmd_if_destroy(adapter,
					adapter->vf_cfg[vf].vf_if_handle,
					vf + 1);

	be_cmd_if_destroy(adapter, adapter->if_handle,  0);

	/* tell fw we're done with firing cmds */
	be_cmd_fw_clean(adapter);
	return 0;
}


#define FW_FILE_HDR_SIGN 	"ServerEngines Corp. "
static bool be_flash_redboot(struct be_adapter *adapter,
			const u8 *p, u32 img_start, int image_size,
			int hdr_size)
{
	u32 crc_offset;
	u8 flashed_crc[4];
	int status;

	crc_offset = hdr_size + img_start + image_size - 4;

	p += crc_offset;

	status = be_cmd_get_flash_crc(adapter, flashed_crc,
			(image_size - 4));
	if (status) {
		dev_err(&adapter->pdev->dev,
		"could not get crc from flash, not flashing redboot\n");
		return false;
	}

	/*update redboot only if crc does not match*/
	if (!memcmp(flashed_crc, p, 4))
		return false;
	else
		return true;
}

static int be_flash_data(struct be_adapter *adapter,
			const struct firmware *fw,
			struct be_dma_mem *flash_cmd, int num_of_images)

{
	int status = 0, i, filehdr_size = 0;
	u32 total_bytes = 0, flash_op;
	int num_bytes;
	const u8 *p = fw->data;
	struct be_cmd_write_flashrom *req = flash_cmd->va;
	const struct flash_comp *pflashcomp;
	int num_comp;

	static const struct flash_comp gen3_flash_types[9] = {
		{ FLASH_iSCSI_PRIMARY_IMAGE_START_g3, IMG_TYPE_ISCSI_ACTIVE,
			FLASH_IMAGE_MAX_SIZE_g3},
		{ FLASH_REDBOOT_START_g3, IMG_TYPE_REDBOOT,
			FLASH_REDBOOT_IMAGE_MAX_SIZE_g3},
		{ FLASH_iSCSI_BIOS_START_g3, IMG_TYPE_BIOS,
			FLASH_BIOS_IMAGE_MAX_SIZE_g3},
		{ FLASH_PXE_BIOS_START_g3, IMG_TYPE_PXE_BIOS,
			FLASH_BIOS_IMAGE_MAX_SIZE_g3},
		{ FLASH_FCoE_BIOS_START_g3, IMG_TYPE_FCOE_BIOS,
			FLASH_BIOS_IMAGE_MAX_SIZE_g3},
		{ FLASH_iSCSI_BACKUP_IMAGE_START_g3, IMG_TYPE_ISCSI_BACKUP,
			FLASH_IMAGE_MAX_SIZE_g3},
		{ FLASH_FCoE_PRIMARY_IMAGE_START_g3, IMG_TYPE_FCOE_FW_ACTIVE,
			FLASH_IMAGE_MAX_SIZE_g3},
		{ FLASH_FCoE_BACKUP_IMAGE_START_g3, IMG_TYPE_FCOE_FW_BACKUP,
			FLASH_IMAGE_MAX_SIZE_g3},
		{ FLASH_NCSI_START_g3, IMG_TYPE_NCSI_FW,
			FLASH_NCSI_IMAGE_MAX_SIZE_g3}
	};
	static const struct flash_comp gen2_flash_types[8] = {
		{ FLASH_iSCSI_PRIMARY_IMAGE_START_g2, IMG_TYPE_ISCSI_ACTIVE,
			FLASH_IMAGE_MAX_SIZE_g2},
		{ FLASH_REDBOOT_START_g2, IMG_TYPE_REDBOOT,
			FLASH_REDBOOT_IMAGE_MAX_SIZE_g2},
		{ FLASH_iSCSI_BIOS_START_g2, IMG_TYPE_BIOS,
			FLASH_BIOS_IMAGE_MAX_SIZE_g2},
		{ FLASH_PXE_BIOS_START_g2, IMG_TYPE_PXE_BIOS,
			FLASH_BIOS_IMAGE_MAX_SIZE_g2},
		{ FLASH_FCoE_BIOS_START_g2, IMG_TYPE_FCOE_BIOS,
			FLASH_BIOS_IMAGE_MAX_SIZE_g2},
		{ FLASH_iSCSI_BACKUP_IMAGE_START_g2, IMG_TYPE_ISCSI_BACKUP,
			FLASH_IMAGE_MAX_SIZE_g2},
		{ FLASH_FCoE_PRIMARY_IMAGE_START_g2, IMG_TYPE_FCOE_FW_ACTIVE,
			FLASH_IMAGE_MAX_SIZE_g2},
		{ FLASH_FCoE_BACKUP_IMAGE_START_g2, IMG_TYPE_FCOE_FW_BACKUP,
			 FLASH_IMAGE_MAX_SIZE_g2}
	};

	if (adapter->generation == BE_GEN3) {
		pflashcomp = gen3_flash_types;
		filehdr_size = sizeof(struct flash_file_hdr_g3);
		num_comp = ARRAY_SIZE(gen3_flash_types);
	} else {
		pflashcomp = gen2_flash_types;
		filehdr_size = sizeof(struct flash_file_hdr_g2);
		num_comp = ARRAY_SIZE(gen2_flash_types);
	}
	for (i = 0; i < num_comp; i++) {
		if ((pflashcomp[i].optype == IMG_TYPE_NCSI_FW) &&
				memcmp(adapter->fw_ver, "3.102.148.0", 11) < 0)
			continue;
		if ((pflashcomp[i].optype == IMG_TYPE_REDBOOT) &&
			(!be_flash_redboot(adapter, fw->data,
			pflashcomp[i].offset, pflashcomp[i].size, filehdr_size +
			(num_of_images * sizeof(struct image_hdr)))))
			continue;
		p = fw->data;
		p += filehdr_size + pflashcomp[i].offset
			+ (num_of_images * sizeof(struct image_hdr));
	if (p + pflashcomp[i].size > fw->data + fw->size)
		return -1;
	total_bytes = pflashcomp[i].size;
		while (total_bytes) {
			if (total_bytes > 32*1024)
				num_bytes = 32*1024;
			else
				num_bytes = total_bytes;
			total_bytes -= num_bytes;

			if (!total_bytes)
				flash_op = FLASHROM_OPER_FLASH;
			else
				flash_op = FLASHROM_OPER_SAVE;
			memcpy(req->params.data_buf, p, num_bytes);
			p += num_bytes;
			status = be_cmd_write_flashrom(adapter, flash_cmd,
				pflashcomp[i].optype, flash_op, num_bytes);
			if (status) {
				dev_err(&adapter->pdev->dev,
					"cmd to write to flash rom failed.\n");
				return -1;
			}
		}
	}
	return 0;
}

static int get_ufigen_type(struct flash_file_hdr_g2 *fhdr)
{
	if (fhdr == NULL)
		return 0;
	if (fhdr->build[0] == '3')
		return BE_GEN3;
	else if (fhdr->build[0] == '2')
		return BE_GEN2;
	else
		return 0;
}

static int lancer_fw_download(struct be_adapter *adapter,
				const struct firmware *fw)
{
#define LANCER_FW_DOWNLOAD_CHUNK      (32 * 1024)
#define LANCER_FW_DOWNLOAD_LOCATION   "/prg"
	struct be_dma_mem flash_cmd;
	const u8 *data_ptr = NULL;
	u8 *dest_image_ptr = NULL;
	size_t image_size = 0;
	u32 chunk_size = 0;
	u32 data_written = 0;
	u32 offset = 0;
	int status = 0;
	u8 add_status = 0;

	if (!IS_ALIGNED(fw->size, sizeof(u32))) {
		dev_err(&adapter->pdev->dev,
			"FW Image not properly aligned. "
			"Length must be 4 byte aligned.\n");
		status = -EINVAL;
		goto lancer_fw_exit;
	}

	flash_cmd.size = sizeof(struct lancer_cmd_req_write_object)
				+ LANCER_FW_DOWNLOAD_CHUNK;
	flash_cmd.va = dma_alloc_coherent(&adapter->pdev->dev, flash_cmd.size,
						&flash_cmd.dma, GFP_KERNEL);
	if (!flash_cmd.va) {
		status = -ENOMEM;
		dev_err(&adapter->pdev->dev,
			"Memory allocation failure while flashing\n");
		goto lancer_fw_exit;
	}

	dest_image_ptr = flash_cmd.va +
				sizeof(struct lancer_cmd_req_write_object);
	image_size = fw->size;
	data_ptr = fw->data;

	while (image_size) {
		chunk_size = min_t(u32, image_size, LANCER_FW_DOWNLOAD_CHUNK);

		/* Copy the image chunk content. */
		memcpy(dest_image_ptr, data_ptr, chunk_size);

		status = lancer_cmd_write_object(adapter, &flash_cmd,
				chunk_size, offset, LANCER_FW_DOWNLOAD_LOCATION,
				&data_written, &add_status);

		if (status)
			break;

		offset += data_written;
		data_ptr += data_written;
		image_size -= data_written;
	}

	if (!status) {
		/* Commit the FW written */
		status = lancer_cmd_write_object(adapter, &flash_cmd,
					0, offset, LANCER_FW_DOWNLOAD_LOCATION,
					&data_written, &add_status);
	}

	dma_free_coherent(&adapter->pdev->dev, flash_cmd.size, flash_cmd.va,
				flash_cmd.dma);
	if (status) {
		dev_err(&adapter->pdev->dev,
			"Firmware load error. "
			"Status code: 0x%x Additional Status: 0x%x\n",
			status, add_status);
		goto lancer_fw_exit;
	}

	dev_info(&adapter->pdev->dev, "Firmware flashed successfully\n");
lancer_fw_exit:
	return status;
}

static int be_fw_download(struct be_adapter *adapter, const struct firmware* fw)
{
	struct flash_file_hdr_g2 *fhdr;
	struct flash_file_hdr_g3 *fhdr3;
	struct image_hdr *img_hdr_ptr = NULL;
	struct be_dma_mem flash_cmd;
	const u8 *p;
	int status = 0, i = 0, num_imgs = 0;

	p = fw->data;
	fhdr = (struct flash_file_hdr_g2 *) p;

	flash_cmd.size = sizeof(struct be_cmd_write_flashrom) + 32*1024;
	flash_cmd.va = dma_alloc_coherent(&adapter->pdev->dev, flash_cmd.size,
					  &flash_cmd.dma, GFP_KERNEL);
	if (!flash_cmd.va) {
		status = -ENOMEM;
		dev_err(&adapter->pdev->dev,
			"Memory allocation failure while flashing\n");
		goto be_fw_exit;
	}

	if ((adapter->generation == BE_GEN3) &&
			(get_ufigen_type(fhdr) == BE_GEN3)) {
		fhdr3 = (struct flash_file_hdr_g3 *) fw->data;
		num_imgs = le32_to_cpu(fhdr3->num_imgs);
		for (i = 0; i < num_imgs; i++) {
			img_hdr_ptr = (struct image_hdr *) (fw->data +
					(sizeof(struct flash_file_hdr_g3) +
					 i * sizeof(struct image_hdr)));
			if (le32_to_cpu(img_hdr_ptr->imageid) == 1)
				status = be_flash_data(adapter, fw, &flash_cmd,
							num_imgs);
		}
	} else if ((adapter->generation == BE_GEN2) &&
			(get_ufigen_type(fhdr) == BE_GEN2)) {
		status = be_flash_data(adapter, fw, &flash_cmd, 0);
	} else {
		dev_err(&adapter->pdev->dev,
			"UFI and Interface are not compatible for flashing\n");
		status = -1;
	}

	dma_free_coherent(&adapter->pdev->dev, flash_cmd.size, flash_cmd.va,
			  flash_cmd.dma);
	if (status) {
		dev_err(&adapter->pdev->dev, "Firmware load error\n");
		goto be_fw_exit;
	}

	dev_info(&adapter->pdev->dev, "Firmware flashed successfully\n");

be_fw_exit:
	return status;
}

int be_load_fw(struct be_adapter *adapter, u8 *fw_file)
{
	const struct firmware *fw;
	int status;

	if (!netif_running(adapter->netdev)) {
		dev_err(&adapter->pdev->dev,
			"Firmware load not allowed (interface is down)\n");
		return -1;
	}

	status = request_firmware(&fw, fw_file, &adapter->pdev->dev);
	if (status)
		goto fw_exit;

	dev_info(&adapter->pdev->dev, "Flashing firmware file %s\n", fw_file);

	if (lancer_chip(adapter))
		status = lancer_fw_download(adapter, fw);
	else
		status = be_fw_download(adapter, fw);

fw_exit:
	release_firmware(fw);
	return status;
}

static struct net_device_ops be_netdev_ops = {
	.ndo_open		= be_open,
	.ndo_stop		= be_close,
	.ndo_start_xmit		= be_xmit,
	.ndo_set_rx_mode	= be_set_multicast_list,
	.ndo_set_mac_address	= be_mac_addr_set,
	.ndo_change_mtu		= be_change_mtu,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_vlan_rx_register	= be_vlan_register,
	.ndo_vlan_rx_add_vid	= be_vlan_add_vid,
	.ndo_vlan_rx_kill_vid	= be_vlan_rem_vid,
	.ndo_set_vf_mac		= be_set_vf_mac,
	.ndo_set_vf_vlan	= be_set_vf_vlan,
	.ndo_set_vf_tx_rate	= be_set_vf_tx_rate,
	.ndo_get_vf_config	= be_get_vf_config
};

static void be_netdev_init(struct net_device *netdev)
{
	struct be_adapter *adapter = netdev_priv(netdev);
	struct be_rx_obj *rxo;
	int i;

	netdev->hw_features |= NETIF_F_SG | NETIF_F_TSO | NETIF_F_TSO6 |
		NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | NETIF_F_RXCSUM |
		NETIF_F_HW_VLAN_TX;
	if (be_multi_rxq(adapter))
		netdev->hw_features |= NETIF_F_RXHASH;

	netdev->features |= netdev->hw_features |
		NETIF_F_HW_VLAN_RX | NETIF_F_HW_VLAN_FILTER;

	netdev->vlan_features |= NETIF_F_SG | NETIF_F_TSO | NETIF_F_TSO6 |
		NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM;

	netdev->flags |= IFF_MULTICAST;

	/* Default settings for Rx and Tx flow control */
	adapter->rx_fc = true;
	adapter->tx_fc = true;

	netif_set_gso_max_size(netdev, 65535);

	BE_SET_NETDEV_OPS(netdev, &be_netdev_ops);

	SET_ETHTOOL_OPS(netdev, &be_ethtool_ops);

	for_all_rx_queues(adapter, rxo, i)
		netif_napi_add(netdev, &rxo->rx_eq.napi, be_poll_rx,
				BE_NAPI_WEIGHT);

	netif_napi_add(netdev, &adapter->tx_eq.napi, be_poll_tx_mcc,
		BE_NAPI_WEIGHT);
}

static void be_unmap_pci_bars(struct be_adapter *adapter)
{
	if (adapter->csr)
		iounmap(adapter->csr);
	if (adapter->db)
		iounmap(adapter->db);
	if (adapter->pcicfg && be_physfn(adapter))
		iounmap(adapter->pcicfg);
}

static int be_map_pci_bars(struct be_adapter *adapter)
{
	u8 __iomem *addr;
	int pcicfg_reg, db_reg;

	if (lancer_chip(adapter)) {
		addr = ioremap_nocache(pci_resource_start(adapter->pdev, 0),
			pci_resource_len(adapter->pdev, 0));
		if (addr == NULL)
			return -ENOMEM;
		adapter->db = addr;
		return 0;
	}

	if (be_physfn(adapter)) {
		addr = ioremap_nocache(pci_resource_start(adapter->pdev, 2),
				pci_resource_len(adapter->pdev, 2));
		if (addr == NULL)
			return -ENOMEM;
		adapter->csr = addr;
	}

	if (adapter->generation == BE_GEN2) {
		pcicfg_reg = 1;
		db_reg = 4;
	} else {
		pcicfg_reg = 0;
		if (be_physfn(adapter))
			db_reg = 4;
		else
			db_reg = 0;
	}
	addr = ioremap_nocache(pci_resource_start(adapter->pdev, db_reg),
				pci_resource_len(adapter->pdev, db_reg));
	if (addr == NULL)
		goto pci_map_err;
	adapter->db = addr;

	if (be_physfn(adapter)) {
		addr = ioremap_nocache(
				pci_resource_start(adapter->pdev, pcicfg_reg),
				pci_resource_len(adapter->pdev, pcicfg_reg));
		if (addr == NULL)
			goto pci_map_err;
		adapter->pcicfg = addr;
	} else
		adapter->pcicfg = adapter->db + SRIOV_VF_PCICFG_OFFSET;

	return 0;
pci_map_err:
	be_unmap_pci_bars(adapter);
	return -ENOMEM;
}


static void be_ctrl_cleanup(struct be_adapter *adapter)
{
	struct be_dma_mem *mem = &adapter->mbox_mem_alloced;

	be_unmap_pci_bars(adapter);

	if (mem->va)
		dma_free_coherent(&adapter->pdev->dev, mem->size, mem->va,
				  mem->dma);

	mem = &adapter->mc_cmd_mem;
	if (mem->va)
		dma_free_coherent(&adapter->pdev->dev, mem->size, mem->va,
				  mem->dma);
}

static int be_ctrl_init(struct be_adapter *adapter)
{
	struct be_dma_mem *mbox_mem_alloc = &adapter->mbox_mem_alloced;
	struct be_dma_mem *mbox_mem_align = &adapter->mbox_mem;
	struct be_dma_mem *mc_cmd_mem = &adapter->mc_cmd_mem;
	int status;

	status = be_map_pci_bars(adapter);
	if (status)
		goto done;

	mbox_mem_alloc->size = sizeof(struct be_mcc_mailbox) + 16;
	mbox_mem_alloc->va = dma_alloc_coherent(&adapter->pdev->dev,
						mbox_mem_alloc->size,
						&mbox_mem_alloc->dma,
						GFP_KERNEL);
	if (!mbox_mem_alloc->va) {
		status = -ENOMEM;
		goto unmap_pci_bars;
	}

	mbox_mem_align->size = sizeof(struct be_mcc_mailbox);
	mbox_mem_align->va = PTR_ALIGN(mbox_mem_alloc->va, 16);
	mbox_mem_align->dma = PTR_ALIGN(mbox_mem_alloc->dma, 16);
	memset(mbox_mem_align->va, 0, sizeof(struct be_mcc_mailbox));

	mc_cmd_mem->size = sizeof(struct be_cmd_req_mcast_mac_config);
	mc_cmd_mem->va = dma_alloc_coherent(&adapter->pdev->dev,
					    mc_cmd_mem->size, &mc_cmd_mem->dma,
					    GFP_KERNEL);
	if (mc_cmd_mem->va == NULL) {
		status = -ENOMEM;
		goto free_mbox;
	}
	memset(mc_cmd_mem->va, 0, mc_cmd_mem->size);

	mutex_init(&adapter->mbox_lock);
	spin_lock_init(&adapter->mcc_lock);
	spin_lock_init(&adapter->mcc_cq_lock);

	init_completion(&adapter->flash_compl);
	pci_save_state(adapter->pdev);
	return 0;

free_mbox:
	dma_free_coherent(&adapter->pdev->dev, mbox_mem_alloc->size,
			  mbox_mem_alloc->va, mbox_mem_alloc->dma);

unmap_pci_bars:
	be_unmap_pci_bars(adapter);

done:
	return status;
}

static void be_stats_cleanup(struct be_adapter *adapter)
{
	struct be_dma_mem *cmd = &adapter->stats_cmd;

	if (cmd->va)
		dma_free_coherent(&adapter->pdev->dev, cmd->size,
				  cmd->va, cmd->dma);
}

static int be_stats_init(struct be_adapter *adapter)
{
	struct be_dma_mem *cmd = &adapter->stats_cmd;

	if (adapter->generation == BE_GEN2) {
		cmd->size = sizeof(struct be_cmd_req_get_stats_v0);
	} else {
		if (lancer_chip(adapter))
			cmd->size = sizeof(struct lancer_cmd_req_pport_stats);
		else
			cmd->size = sizeof(struct be_cmd_req_get_stats_v1);
	}
	cmd->va = dma_alloc_coherent(&adapter->pdev->dev, cmd->size, &cmd->dma,
				     GFP_KERNEL);
	if (cmd->va == NULL)
		return -1;
	memset(cmd->va, 0, cmd->size);
	return 0;
}

static void __devexit be_remove(struct pci_dev *pdev)
{
	struct be_adapter *adapter = pci_get_drvdata(pdev);

	if (!adapter)
		return;

	cancel_delayed_work_sync(&adapter->work);

	unregister_netdev(adapter->netdev);

	be_clear(adapter);

	be_stats_cleanup(adapter);

	be_ctrl_cleanup(adapter);

	kfree(adapter->vf_cfg);
	be_sriov_disable(adapter);

	be_msix_disable(adapter);

	pci_set_drvdata(pdev, NULL);
	pci_release_regions(pdev);
	pci_disable_device(pdev);

	free_netdev(adapter->netdev);
}

static int be_get_config(struct be_adapter *adapter)
{
	int status;
	u8 mac[ETH_ALEN];

	status = be_cmd_get_fw_ver(adapter, adapter->fw_ver);
	if (status)
		return status;

	status = be_cmd_query_fw_cfg(adapter, &adapter->port_num,
			&adapter->function_mode, &adapter->function_caps);
	if (status)
		return status;

	memset(mac, 0, ETH_ALEN);

	/* A default permanent address is given to each VF for Lancer*/
	if (be_physfn(adapter) || lancer_chip(adapter)) {
		status = be_cmd_mac_addr_query(adapter, mac,
			MAC_ADDRESS_TYPE_NETWORK, true /*permanent */, 0);

		if (status)
			return status;

		if (!is_valid_ether_addr(mac))
			return -EADDRNOTAVAIL;

		memcpy(adapter->netdev->dev_addr, mac, ETH_ALEN);
		memcpy(adapter->netdev->perm_addr, mac, ETH_ALEN);
	}

	if (adapter->function_mode & 0x400)
		adapter->max_vlans = BE_NUM_VLANS_SUPPORTED/4;
	else
		adapter->max_vlans = BE_NUM_VLANS_SUPPORTED;

	status = be_cmd_get_cntl_attributes(adapter);
	if (status)
		return status;

	be_cmd_check_native_mode(adapter);

	if ((num_vfs && adapter->sriov_enabled) ||
		(adapter->function_mode & 0x400) ||
		lancer_chip(adapter) || !be_physfn(adapter)) {
		adapter->num_tx_qs = 1;
		netif_set_real_num_tx_queues(adapter->netdev,
			adapter->num_tx_qs);
	} else {
		adapter->num_tx_qs = MAX_TX_QS;
	}

	return 0;
}

static int be_dev_family_check(struct be_adapter *adapter)
{
	struct pci_dev *pdev = adapter->pdev;
	u32 sli_intf = 0, if_type;

	switch (pdev->device) {
	case BE_DEVICE_ID1:
	case OC_DEVICE_ID1:
		adapter->generation = BE_GEN2;
		break;
	case BE_DEVICE_ID2:
	case OC_DEVICE_ID2:
		adapter->generation = BE_GEN3;
		break;
	case OC_DEVICE_ID3:
	case OC_DEVICE_ID4:
		pci_read_config_dword(pdev, SLI_INTF_REG_OFFSET, &sli_intf);
		if_type = (sli_intf & SLI_INTF_IF_TYPE_MASK) >>
						SLI_INTF_IF_TYPE_SHIFT;

		if (((sli_intf & SLI_INTF_VALID_MASK) != SLI_INTF_VALID) ||
			if_type != 0x02) {
			dev_err(&pdev->dev, "SLI_INTF reg val is not valid\n");
			return -EINVAL;
		}
		adapter->sli_family = ((sli_intf & SLI_INTF_FAMILY_MASK) >>
					 SLI_INTF_FAMILY_SHIFT);
		adapter->generation = BE_GEN3;
		break;
	default:
		adapter->generation = 0;
	}
	return 0;
}

static int lancer_wait_ready(struct be_adapter *adapter)
{
#define SLIPORT_READY_TIMEOUT 500
	u32 sliport_status;
	int status = 0, i;

	for (i = 0; i < SLIPORT_READY_TIMEOUT; i++) {
		sliport_status = ioread32(adapter->db + SLIPORT_STATUS_OFFSET);
		if (sliport_status & SLIPORT_STATUS_RDY_MASK)
			break;

		msleep(20);
	}

	if (i == SLIPORT_READY_TIMEOUT)
		status = -1;

	return status;
}

static int lancer_test_and_set_rdy_state(struct be_adapter *adapter)
{
	int status;
	u32 sliport_status, err, reset_needed;
	status = lancer_wait_ready(adapter);
	if (!status) {
		sliport_status = ioread32(adapter->db + SLIPORT_STATUS_OFFSET);
		err = sliport_status & SLIPORT_STATUS_ERR_MASK;
		reset_needed = sliport_status & SLIPORT_STATUS_RN_MASK;
		if (err && reset_needed) {
			iowrite32(SLI_PORT_CONTROL_IP_MASK,
					adapter->db + SLIPORT_CONTROL_OFFSET);

			/* check adapter has corrected the error */
			status = lancer_wait_ready(adapter);
			sliport_status = ioread32(adapter->db +
							SLIPORT_STATUS_OFFSET);
			sliport_status &= (SLIPORT_STATUS_ERR_MASK |
						SLIPORT_STATUS_RN_MASK);
			if (status || sliport_status)
				status = -1;
		} else if (err || reset_needed) {
			status = -1;
		}
	}
	return status;
}

static int __devinit be_probe(struct pci_dev *pdev,
			const struct pci_device_id *pdev_id)
{
	int status = 0;
	struct be_adapter *adapter;
	struct net_device *netdev;

	status = pci_enable_device(pdev);
	if (status)
		goto do_none;

	status = pci_request_regions(pdev, DRV_NAME);
	if (status)
		goto disable_dev;
	pci_set_master(pdev);

	netdev = alloc_etherdev_mq(sizeof(struct be_adapter), MAX_TX_QS);
	if (netdev == NULL) {
		status = -ENOMEM;
		goto rel_reg;
	}
	adapter = netdev_priv(netdev);
	adapter->pdev = pdev;
	pci_set_drvdata(pdev, adapter);

	status = be_dev_family_check(adapter);
	if (status)
		goto free_netdev;

	adapter->netdev = netdev;
	SET_NETDEV_DEV(netdev, &pdev->dev);

	status = dma_set_mask(&pdev->dev, DMA_BIT_MASK(64));
	if (!status) {
		netdev->features |= NETIF_F_HIGHDMA;
	} else {
		status = dma_set_mask(&pdev->dev, DMA_BIT_MASK(32));
		if (status) {
			dev_err(&pdev->dev, "Could not set PCI DMA Mask\n");
			goto free_netdev;
		}
	}

	be_sriov_enable(adapter);
	if (adapter->sriov_enabled) {
		adapter->vf_cfg = kcalloc(num_vfs,
			sizeof(struct be_vf_cfg), GFP_KERNEL);

		if (!adapter->vf_cfg)
			goto free_netdev;
	}

	status = be_ctrl_init(adapter);
	if (status)
		goto free_vf_cfg;

	if (lancer_chip(adapter)) {
		status = lancer_test_and_set_rdy_state(adapter);
		if (status) {
			dev_err(&pdev->dev, "Adapter in non recoverable error\n");
			goto ctrl_clean;
		}
	}

	/* sync up with fw's ready state */
	if (be_physfn(adapter)) {
		status = be_cmd_POST(adapter);
		if (status)
			goto ctrl_clean;
	}

	/* tell fw we're ready to fire cmds */
	status = be_cmd_fw_init(adapter);
	if (status)
		goto ctrl_clean;

	status = be_cmd_reset_function(adapter);
	if (status)
		goto ctrl_clean;

	status = be_stats_init(adapter);
	if (status)
		goto ctrl_clean;

	status = be_get_config(adapter);
	if (status)
		goto stats_clean;

	be_msix_enable(adapter);

	INIT_DELAYED_WORK(&adapter->work, be_worker);

	status = be_setup(adapter);
	if (status)
		goto msix_disable;

	be_netdev_init(netdev);
	status = register_netdev(netdev);
	if (status != 0)
		goto unsetup;
	netif_carrier_off(netdev);

	if (be_physfn(adapter) && adapter->sriov_enabled) {
		u8 mac_speed;
		bool link_up;
		u16 vf, lnk_speed;

		if (!lancer_chip(adapter)) {
			status = be_vf_eth_addr_config(adapter);
			if (status)
				goto unreg_netdev;
		}

		for (vf = 0; vf < num_vfs; vf++) {
			status = be_cmd_link_status_query(adapter, &link_up,
					&mac_speed, &lnk_speed, vf + 1);
			if (!status)
				adapter->vf_cfg[vf].vf_tx_rate = lnk_speed * 10;
			else
				goto unreg_netdev;
		}
	}

	dev_info(&pdev->dev, "%s port %d\n", nic_name(pdev), adapter->port_num);
	/* By default all priorities are enabled.
	 * Needed in case of no GRP5 evt support
	 */
	adapter->vlan_prio_bmap = 0xff;

	schedule_delayed_work(&adapter->work, msecs_to_jiffies(100));
	return 0;

unreg_netdev:
	unregister_netdev(netdev);
unsetup:
	be_clear(adapter);
msix_disable:
	be_msix_disable(adapter);
stats_clean:
	be_stats_cleanup(adapter);
ctrl_clean:
	be_ctrl_cleanup(adapter);
free_vf_cfg:
	kfree(adapter->vf_cfg);
free_netdev:
	be_sriov_disable(adapter);
	free_netdev(netdev);
	pci_set_drvdata(pdev, NULL);
rel_reg:
	pci_release_regions(pdev);
disable_dev:
	pci_disable_device(pdev);
do_none:
	dev_err(&pdev->dev, "%s initialization failed\n", nic_name(pdev));
	return status;
}

static int be_suspend(struct pci_dev *pdev, pm_message_t state)
{
	struct be_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev =  adapter->netdev;

	cancel_delayed_work_sync(&adapter->work);
	if (adapter->wol)
		be_setup_wol(adapter, true);

	netif_device_detach(netdev);
	if (netif_running(netdev)) {
		rtnl_lock();
		be_close(netdev);
		rtnl_unlock();
	}
	be_cmd_get_flow_control(adapter, &adapter->tx_fc, &adapter->rx_fc);
	be_clear(adapter);

	be_msix_disable(adapter);
	pci_save_state(pdev);
	pci_disable_device(pdev);
	pci_set_power_state(pdev, pci_choose_state(pdev, state));
	return 0;
}

static int be_resume(struct pci_dev *pdev)
{
	int status = 0;
	struct be_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev =  adapter->netdev;

	netif_device_detach(netdev);

	status = pci_enable_device(pdev);
	if (status)
		return status;

	pci_set_power_state(pdev, 0);
	pci_restore_state(pdev);

	be_msix_enable(adapter);
	/* tell fw we're ready to fire cmds */
	status = be_cmd_fw_init(adapter);
	if (status)
		return status;

	be_setup(adapter);
	if (netif_running(netdev)) {
		rtnl_lock();
		be_open(netdev);
		rtnl_unlock();
	}
	netif_device_attach(netdev);

	if (adapter->wol)
		be_setup_wol(adapter, false);

	schedule_delayed_work(&adapter->work, msecs_to_jiffies(100));
	return 0;
}

/*
 * An FLR will stop BE from DMAing any data.
 */
static void be_shutdown(struct pci_dev *pdev)
{
	struct be_adapter *adapter = pci_get_drvdata(pdev);

	if (!adapter)
		return;

	cancel_delayed_work_sync(&adapter->work);

	netif_device_detach(adapter->netdev);

	if (adapter->wol)
		be_setup_wol(adapter, true);

	be_cmd_reset_function(adapter);

	pci_disable_device(pdev);
}

static pci_ers_result_t be_eeh_err_detected(struct pci_dev *pdev,
				pci_channel_state_t state)
{
	struct be_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev =  adapter->netdev;

	dev_err(&adapter->pdev->dev, "EEH error detected\n");

	adapter->eeh_err = true;

	netif_device_detach(netdev);

	if (netif_running(netdev)) {
		rtnl_lock();
		be_close(netdev);
		rtnl_unlock();
	}
	be_clear(adapter);

	if (state == pci_channel_io_perm_failure)
		return PCI_ERS_RESULT_DISCONNECT;

	pci_disable_device(pdev);

	return PCI_ERS_RESULT_NEED_RESET;
}

static pci_ers_result_t be_eeh_reset(struct pci_dev *pdev)
{
	struct be_adapter *adapter = pci_get_drvdata(pdev);
	int status;

	dev_info(&adapter->pdev->dev, "EEH reset\n");
	adapter->eeh_err = false;

	status = pci_enable_device(pdev);
	if (status)
		return PCI_ERS_RESULT_DISCONNECT;

	pci_set_master(pdev);
	pci_set_power_state(pdev, 0);
	pci_restore_state(pdev);

	/* Check if card is ok and fw is ready */
	status = be_cmd_POST(adapter);
	if (status)
		return PCI_ERS_RESULT_DISCONNECT;

	return PCI_ERS_RESULT_RECOVERED;
}

static void be_eeh_resume(struct pci_dev *pdev)
{
	int status = 0;
	struct be_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev =  adapter->netdev;

	dev_info(&adapter->pdev->dev, "EEH resume\n");

	pci_save_state(pdev);

	/* tell fw we're ready to fire cmds */
	status = be_cmd_fw_init(adapter);
	if (status)
		goto err;

	status = be_setup(adapter);
	if (status)
		goto err;

	if (netif_running(netdev)) {
		status = be_open(netdev);
		if (status)
			goto err;
	}
	netif_device_attach(netdev);
	return;
err:
	dev_err(&adapter->pdev->dev, "EEH resume failed\n");
}

static struct pci_error_handlers be_eeh_handlers = {
	.error_detected = be_eeh_err_detected,
	.slot_reset = be_eeh_reset,
	.resume = be_eeh_resume,
};

static struct pci_driver be_driver = {
	.name = DRV_NAME,
	.id_table = be_dev_ids,
	.probe = be_probe,
	.remove = be_remove,
	.suspend = be_suspend,
	.resume = be_resume,
	.shutdown = be_shutdown,
	.err_handler = &be_eeh_handlers
};

static int __init be_init_module(void)
{
	if (rx_frag_size != 8192 && rx_frag_size != 4096 &&
	    rx_frag_size != 2048) {
		printk(KERN_WARNING DRV_NAME
			" : Module param rx_frag_size must be 2048/4096/8192."
			" Using 2048\n");
		rx_frag_size = 2048;
	}

	return pci_register_driver(&be_driver);
}
module_init(be_init_module);

static void __exit be_exit_module(void)
{
	pci_unregister_driver(&be_driver);
}
module_exit(be_exit_module);
