/*
 * Intel Wireless WiMAX Connection 2400m
 * Glue with the networking stack
 *
 *
 * Copyright (C) 2007 Intel Corporation <linux-wimax@intel.com>
 * Yanir Lubetkin <yanirx.lubetkin@intel.com>
 * Inaky Perez-Gonzalez <inaky.perez-gonzalez@intel.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 *
 * This implements an ethernet device for the i2400m.
 *
 * We fake being an ethernet device to simplify the support from user
 * space and from the other side. The world is (sadly) configured to
 * take in only Ethernet devices...
 *
 * Because of this, currently there is an copy-each-rxed-packet
 * overhead on the RX path. Each IP packet has to be reallocated to
 * add an ethernet header (as there is no space in what we get from
 * the device). This is a known drawback and coming versions of the
 * device's firmware are being changed to add header space that can be
 * used to insert the ethernet header without having to reallocate and
 * copy.
 *
 * TX error handling is tricky; because we have to FIFO/queue the
 * buffers for transmission (as the hardware likes it aggregated), we
 * just give the skb to the TX subsystem and by the time it is
 * transmitted, we have long forgotten about it. So we just don't care
 * too much about it.
 *
 * Note that when the device is in idle mode with the basestation, we
 * need to negotiate coming back up online. That involves negotiation
 * and possible user space interaction. Thus, we defer to a workqueue
 * to do all that. By default, we only queue a single packet and drop
 * the rest, as potentially the time to go back from idle to normal is
 * long.
 *
 * ROADMAP
 *
 * i2400m_open         Called on ifconfig up
 * i2400m_stop         Called on ifconfig down
 *
 * i2400m_hard_start_xmit Called by the network stack to send a packet
 *   i2400m_net_wake_tx	  Wake up device from basestation-IDLE & TX
 *     i2400m_wake_tx_work
 *       i2400m_cmd_exit_idle
 *       i2400m_tx
 *   i2400m_net_tx        TX a data frame
 *     i2400m_tx
 *
 * i2400m_change_mtu      Called on ifconfig mtu XXX
 *
 * i2400m_tx_timeout      Called when the device times out
 *
 * i2400m_net_rx          Called by the RX code when a data frame is
 *                        available.
 * i2400m_netdev_setup    Called to setup all the netdev stuff from
 *                        alloc_netdev.
 */
#include <linux/if_arp.h>
#include <linux/netdevice.h>
#include "i2400m.h"


#define D_SUBMODULE netdev
#include "debug-levels.h"

enum {
/* netdev interface */
	/*
	 * Out of NWG spec (R1_v1.2.2), 3.3.3 ASN Bearer Plane MTU Size
	 *
	 * The MTU is 1400 or less
	 */
	I2400M_MAX_MTU = 1400,
	I2400M_TX_TIMEOUT = HZ,
	I2400M_TX_QLEN = 5,
};


static
int i2400m_open(struct net_device *net_dev)
{
	int result;
	struct i2400m *i2400m = net_dev_to_i2400m(net_dev);
	struct device *dev = i2400m_dev(i2400m);

	d_fnstart(3, dev, "(net_dev %p [i2400m %p])\n", net_dev, i2400m);
	if (i2400m->ready == 0) {
		dev_err(dev, "Device is still initializing\n");
		result = -EBUSY;
	} else
		result = 0;
	d_fnend(3, dev, "(net_dev %p [i2400m %p]) = %d\n",
		net_dev, i2400m, result);
	return result;
}


/*
 *
 * On kernel versions where cancel_work_sync() didn't return anything,
 * we rely on wake_tx_skb() being non-NULL.
 */
static
int i2400m_stop(struct net_device *net_dev)
{
	struct i2400m *i2400m = net_dev_to_i2400m(net_dev);
	struct device *dev = i2400m_dev(i2400m);

	d_fnstart(3, dev, "(net_dev %p [i2400m %p])\n", net_dev, i2400m);
	/* See i2400m_hard_start_xmit(), references are taken there
	 * and here we release them if the work was still
	 * pending. Note we can't differentiate work not pending vs
	 * never scheduled, so the NULL check does that. */
	if (cancel_work_sync(&i2400m->wake_tx_ws) == 0
	    && i2400m->wake_tx_skb != NULL) {
		unsigned long flags;
		struct sk_buff *wake_tx_skb;
		spin_lock_irqsave(&i2400m->tx_lock, flags);
		wake_tx_skb = i2400m->wake_tx_skb;	/* compat help */
		i2400m->wake_tx_skb = NULL;	/* compat help */
		spin_unlock_irqrestore(&i2400m->tx_lock, flags);
		i2400m_put(i2400m);
		kfree_skb(wake_tx_skb);
	}
	d_fnend(3, dev, "(net_dev %p [i2400m %p]) = 0\n", net_dev, i2400m);
	return 0;
}


/*
 * Wake up the device and transmit a held SKB, then restart the net queue
 *
 * When the device goes into basestation-idle mode, we need to tell it
 * to exit that mode; it will negotiate with the base station, user
 * space may have to intervene to rehandshake crypto and then tell us
 * when it is ready to transmit the packet we have "queued". Still we
 * need to give it sometime after it reports being ok.
 *
 * On error, there is not much we can do. If the error was on TX, we
 * still wake the queue up to see if the next packet will be luckier.
 *
 * If _cmd_exit_idle() fails...well, it could be many things; most
 * commonly it is that something else took the device out of IDLE mode
 * (for example, the base station). In that case we get an -EILSEQ and
 * we are just going to ignore that one. If the device is back to
 * connected, then fine -- if it is someother state, the packet will
 * be dropped anyway.
 */
void i2400m_wake_tx_work(struct work_struct *ws)
{
	int result;
	struct i2400m *i2400m = container_of(ws, struct i2400m, wake_tx_ws);
	struct device *dev = i2400m_dev(i2400m);
	struct sk_buff *skb = i2400m->wake_tx_skb;
	unsigned long flags;

	spin_lock_irqsave(&i2400m->tx_lock, flags);
	skb = i2400m->wake_tx_skb;
	i2400m->wake_tx_skb = NULL;
	spin_unlock_irqrestore(&i2400m->tx_lock, flags);

	d_fnstart(3, dev, "(ws %p i2400m %p skb %p)\n", ws, i2400m, skb);
	result = -EINVAL;
	if (skb == NULL) {
		dev_err(dev, "WAKE&TX: skb dissapeared!\n");
		goto out_put;
	}
	result = i2400m_cmd_exit_idle(i2400m);
	if (result == -EILSEQ)
		result = 0;
	if (result < 0) {
		dev_err(dev, "WAKE&TX: device didn't get out of idle: "
			"%d\n", result);
			goto error;
	}
	result = wait_event_timeout(i2400m->state_wq,
				    i2400m->state != I2400M_SS_IDLE, 5 * HZ);
	if (result == 0)
		result = -ETIMEDOUT;
	if (result < 0) {
		dev_err(dev, "WAKE&TX: error waiting for device to exit IDLE: "
			"%d\n", result);
		goto error;
	}
	msleep(20);	/* device still needs some time or it drops it */
	result = i2400m_tx(i2400m, skb->data, skb->len, I2400M_PT_DATA);
	netif_wake_queue(i2400m->wimax_dev.net_dev);
error:
	kfree_skb(skb);	/* refcount transferred by _hard_start_xmit() */
out_put:
	i2400m_put(i2400m);
	d_fnend(3, dev, "(ws %p i2400m %p skb %p) = void [%d]\n",
		ws, i2400m, skb, result);
}


/*
 * Prepare the data payload TX header
 *
 * The i2400m expects a 4 byte header in front of a data packet.
 *
 * Because we pretend to be an ethernet device, this packet comes with
 * an ethernet header. Pull it and push our header.
 */
static
void i2400m_tx_prep_header(struct sk_buff *skb)
{
	struct i2400m_pl_data_hdr *pl_hdr;
	skb_pull(skb, ETH_HLEN);
	pl_hdr = (struct i2400m_pl_data_hdr *) skb_push(skb, sizeof(*pl_hdr));
	pl_hdr->reserved = 0;
}


/*
 * TX an skb to an idle device
 *
 * When the device is in basestation-idle mode, we need to wake it up
 * and then TX. So we queue a work_struct for doing so.
 *
 * We need to get an extra ref for the skb (so it is not dropped), as
 * well as be careful not to queue more than one request (won't help
 * at all). If more than one request comes or there are errors, we
 * just drop the packets (see i2400m_hard_start_xmit()).
 */
static
int i2400m_net_wake_tx(struct i2400m *i2400m, struct net_device *net_dev,
		       struct sk_buff *skb)
{
	int result;
	struct device *dev = i2400m_dev(i2400m);
	unsigned long flags;

	d_fnstart(3, dev, "(skb %p net_dev %p)\n", skb, net_dev);
	if (net_ratelimit()) {
		d_printf(3, dev, "WAKE&NETTX: "
			 "skb %p sending %d bytes to radio\n",
			 skb, skb->len);
		d_dump(4, dev, skb->data, skb->len);
	}
	/* We hold a ref count for i2400m and skb, so when
	 * stopping() the device, we need to cancel that work
	 * and if pending, release those resources. */
	result = 0;
	spin_lock_irqsave(&i2400m->tx_lock, flags);
	if (!work_pending(&i2400m->wake_tx_ws)) {
		netif_stop_queue(net_dev);
		i2400m_get(i2400m);
		i2400m->wake_tx_skb = skb_get(skb);	/* transfer ref count */
		i2400m_tx_prep_header(skb);
		result = schedule_work(&i2400m->wake_tx_ws);
		WARN_ON(result == 0);
	}
	spin_unlock_irqrestore(&i2400m->tx_lock, flags);
	if (result == 0) {
		/* Yes, this happens even if we stopped the
		 * queue -- blame the queue disciplines that
		 * queue without looking -- I guess there is a reason
		 * for that. */
		if (net_ratelimit())
			d_printf(1, dev, "NETTX: device exiting idle, "
				 "dropping skb %p, queue running %d\n",
				 skb, netif_queue_stopped(net_dev));
		result = -EBUSY;
	}
	d_fnend(3, dev, "(skb %p net_dev %p) = %d\n", skb, net_dev, result);
	return result;
}


/*
 * Transmit a packet to the base station on behalf of the network stack.
 *
 * Returns: 0 if ok, < 0 errno code on error.
 *
 * We need to pull the ethernet header and add the hardware header,
 * which is currently set to all zeroes and reserved.
 */
static
int i2400m_net_tx(struct i2400m *i2400m, struct net_device *net_dev,
		  struct sk_buff *skb)
{
	int result;
	struct device *dev = i2400m_dev(i2400m);

	d_fnstart(3, dev, "(i2400m %p net_dev %p skb %p)\n",
		  i2400m, net_dev, skb);
	/* FIXME: check eth hdr, only IPv4 is routed by the device as of now */
	net_dev->trans_start = jiffies;
	i2400m_tx_prep_header(skb);
	d_printf(3, dev, "NETTX: skb %p sending %d bytes to radio\n",
		 skb, skb->len);
	d_dump(4, dev, skb->data, skb->len);
	result = i2400m_tx(i2400m, skb->data, skb->len, I2400M_PT_DATA);
	d_fnend(3, dev, "(i2400m %p net_dev %p skb %p) = %d\n",
		i2400m, net_dev, skb, result);
	return result;
}


/*
 * Transmit a packet to the base station on behalf of the network stack
 *
 *
 * Returns: NETDEV_TX_OK (always, even in case of error)
 *
 * In case of error, we just drop it. Reasons:
 *
 *  - we add a hw header to each skb, and if the network stack
 *    retries, we have no way to know if that skb has it or not.
 *
 *  - network protocols have their own drop-recovery mechanisms
 *
 *  - there is not much else we can do
 *
 * If the device is idle, we need to wake it up; that is an operation
 * that will sleep. See i2400m_net_wake_tx() for details.
 */
static
int i2400m_hard_start_xmit(struct sk_buff *skb,
			   struct net_device *net_dev)
{
	int result;
	struct i2400m *i2400m = net_dev_to_i2400m(net_dev);
	struct device *dev = i2400m_dev(i2400m);

	d_fnstart(3, dev, "(skb %p net_dev %p)\n", skb, net_dev);
	if (i2400m->state == I2400M_SS_IDLE)
		result = i2400m_net_wake_tx(i2400m, net_dev, skb);
	else
		result = i2400m_net_tx(i2400m, net_dev, skb);
	if (result <  0)
		net_dev->stats.tx_dropped++;
	else {
		net_dev->stats.tx_packets++;
		net_dev->stats.tx_bytes += skb->len;
	}
	kfree_skb(skb);
	result = NETDEV_TX_OK;
	d_fnend(3, dev, "(skb %p net_dev %p) = %d\n", skb, net_dev, result);
	return result;
}


static
int i2400m_change_mtu(struct net_device *net_dev, int new_mtu)
{
	int result;
	struct i2400m *i2400m = net_dev_to_i2400m(net_dev);
	struct device *dev = i2400m_dev(i2400m);

	if (new_mtu >= I2400M_MAX_MTU) {
		dev_err(dev, "Cannot change MTU to %d (max is %d)\n",
			new_mtu, I2400M_MAX_MTU);
		result = -EINVAL;
	} else {
		net_dev->mtu = new_mtu;
		result = 0;
	}
	return result;
}


static
void i2400m_tx_timeout(struct net_device *net_dev)
{
	/*
	 * We might want to kick the device
	 *
	 * There is not much we can do though, as the device requires
	 * that we send the data aggregated. By the time we receive
	 * this, there might be data pending to be sent or not...
	 */
	net_dev->stats.tx_errors++;
	return;
}


/*
 * Create a fake ethernet header
 *
 * For emulating an ethernet device, every received IP header has to
 * be prefixed with an ethernet header.
 *
 * What we receive has (potentially) many IP packets concatenated with
 * no ETH_HLEN bytes prefixed. Thus there is no space for an eth
 * header.
 *
 * We would have to reallocate or do ugly fragment tricks in order to
 * add it.
 *
 * But what we do is use the header space of the RX transaction
 * (*msg_hdr) as we don't need it anymore; then we'll point all the
 * data skbs there, as they share the same backing store.
 *
 * We only support IPv4 for v3 firmware.
 */
static
void i2400m_rx_fake_eth_header(struct net_device *net_dev,
			       void *_eth_hdr)
{
	struct ethhdr *eth_hdr = _eth_hdr;

	memcpy(eth_hdr->h_dest, net_dev->dev_addr, sizeof(eth_hdr->h_dest));
	memset(eth_hdr->h_source, 0, sizeof(eth_hdr->h_dest));
	eth_hdr->h_proto = __constant_cpu_to_be16(ETH_P_IP);
}


/*
 * i2400m_net_rx - pass a network packet to the stack
 *
 * @i2400m: device instance
 * @skb_rx: the skb where the buffer pointed to by @buf is
 * @i: 1 if payload is the only one
 * @buf: pointer to the buffer containing the data
 * @len: buffer's length
 *
 * We just clone the skb and set it up so that it's skb->data pointer
 * points to "buf" and it's length.
 *
 * Note that if the payload is the last (or the only one) in a
 * multi-payload message, we don't clone the SKB but just reuse it.
 *
 * This function is normally run from a thread context. However, we
 * still use netif_rx() instead of netif_receive_skb() as was
 * recommended in the mailing list. Reason is in some stress tests
 * when sending/receiving a lot of data we seem to hit a softlock in
 * the kernel's TCP implementation [aroudn tcp_delay_timer()]. Using
 * netif_rx() took care of the issue.
 *
 * This is, of course, still open to do more research on why running
 * with netif_receive_skb() hits this softlock. FIXME.
 *
 * FIXME: currently we don't do any efforts at distinguishing if what
 * we got was an IPv4 or IPv6 header, to setup the protocol field
 * correctly.
 */
void i2400m_net_rx(struct i2400m *i2400m, struct sk_buff *skb_rx,
		   unsigned i, const void *buf, int buf_len)
{
	struct net_device *net_dev = i2400m->wimax_dev.net_dev;
	struct device *dev = i2400m_dev(i2400m);
	struct sk_buff *skb;

	d_fnstart(2, dev, "(i2400m %p buf %p buf_len %d)\n",
		  i2400m, buf, buf_len);
	if (i) {
		skb = skb_get(skb_rx);
		d_printf(2, dev, "RX: reusing first payload skb %p\n", skb);
		skb_pull(skb, buf - (void *) skb->data);
		skb_trim(skb, (void *) skb_end_pointer(skb) - buf);
	} else {
		/* Yes, this is bad -- a lot of overhead -- see
		 * comments at the top of the file */
		skb = __netdev_alloc_skb(net_dev, buf_len, GFP_KERNEL);
		if (skb == NULL) {
			dev_err(dev, "NETRX: no memory to realloc skb\n");
			net_dev->stats.rx_dropped++;
			goto error_skb_realloc;
		}
		memcpy(skb_put(skb, buf_len), buf, buf_len);
	}
	i2400m_rx_fake_eth_header(i2400m->wimax_dev.net_dev,
				  skb->data - ETH_HLEN);
	skb_set_mac_header(skb, -ETH_HLEN);
	skb->dev = i2400m->wimax_dev.net_dev;
	skb->protocol = htons(ETH_P_IP);
	net_dev->stats.rx_packets++;
	net_dev->stats.rx_bytes += buf_len;
	d_printf(3, dev, "NETRX: receiving %d bytes to network stack\n",
		buf_len);
	d_dump(4, dev, buf, buf_len);
	netif_rx_ni(skb);	/* see notes in function header */
error_skb_realloc:
	d_fnend(2, dev, "(i2400m %p buf %p buf_len %d) = void\n",
		i2400m, buf, buf_len);
}


/**
 * i2400m_netdev_setup - Setup setup @net_dev's i2400m private data
 *
 * Called by alloc_netdev()
 */
void i2400m_netdev_setup(struct net_device *net_dev)
{
	d_fnstart(3, NULL, "(net_dev %p)\n", net_dev);
	ether_setup(net_dev);
	net_dev->mtu = I2400M_MAX_MTU;
	net_dev->tx_queue_len = I2400M_TX_QLEN;
	net_dev->features =
		  NETIF_F_VLAN_CHALLENGED
		| NETIF_F_HIGHDMA;
	net_dev->flags =
		IFF_NOARP		/* i2400m is apure IP device */
		& (~IFF_BROADCAST	/* i2400m is P2P */
		   & ~IFF_MULTICAST);
	net_dev->watchdog_timeo = I2400M_TX_TIMEOUT;
	net_dev->open = i2400m_open;
	net_dev->stop = i2400m_stop;
	net_dev->hard_start_xmit = i2400m_hard_start_xmit;
	net_dev->change_mtu = i2400m_change_mtu;
	net_dev->tx_timeout = i2400m_tx_timeout;
	d_fnend(3, NULL, "(net_dev %p) = void\n", net_dev);
}
EXPORT_SYMBOL_GPL(i2400m_netdev_setup);

