#include <linux/err.h>
#include <linux/igmp.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/rculist.h>
#include <linux/skbuff.h>
#include <net/ip.h>
#include <net/netlink.h>
#if IS_ENABLED(CONFIG_IPV6)
#include <net/ipv6.h>
#endif

#include "br_private.h"

static int br_rports_fill_info(struct sk_buff *skb, struct netlink_callback *cb,
			       struct net_device *dev)
{
	struct net_bridge *br = netdev_priv(dev);
	struct net_bridge_port *p;
	struct hlist_node *n;
	struct nlattr *nest;

	if (!br->multicast_router || hlist_empty(&br->router_list))
		return 0;

	nest = nla_nest_start(skb, MDBA_ROUTER);
	if (nest == NULL)
		return -EMSGSIZE;

	hlist_for_each_entry_rcu(p, n, &br->router_list, rlist) {
		if (p && nla_put_u32(skb, MDBA_ROUTER_PORT, p->dev->ifindex))
			goto fail;
	}

	nla_nest_end(skb, nest);
	return 0;
fail:
	nla_nest_cancel(skb, nest);
	return -EMSGSIZE;
}

static int br_mdb_fill_info(struct sk_buff *skb, struct netlink_callback *cb,
			    struct net_device *dev)
{
	struct net_bridge *br = netdev_priv(dev);
	struct net_bridge_mdb_htable *mdb;
	struct nlattr *nest, *nest2;
	int i, err = 0;
	int idx = 0, s_idx = cb->args[1];

	if (br->multicast_disabled)
		return 0;

	mdb = rcu_dereference(br->mdb);
	if (!mdb)
		return 0;

	nest = nla_nest_start(skb, MDBA_MDB);
	if (nest == NULL)
		return -EMSGSIZE;

	for (i = 0; i < mdb->max; i++) {
		struct hlist_node *h;
		struct net_bridge_mdb_entry *mp;
		struct net_bridge_port_group *p, **pp;
		struct net_bridge_port *port;

		hlist_for_each_entry_rcu(mp, h, &mdb->mhash[i], hlist[mdb->ver]) {
			if (idx < s_idx)
				goto skip;

			nest2 = nla_nest_start(skb, MDBA_MDB_ENTRY);
			if (nest2 == NULL) {
				err = -EMSGSIZE;
				goto out;
			}

			for (pp = &mp->ports;
			     (p = rcu_dereference(*pp)) != NULL;
			      pp = &p->next) {
				port = p->port;
				if (port) {
					struct br_mdb_entry e;
					e.ifindex = port->dev->ifindex;
					e.addr.u.ip4 = p->addr.u.ip4;
#if IS_ENABLED(CONFIG_IPV6)
					e.addr.u.ip6 = p->addr.u.ip6;
#endif
					e.addr.proto = p->addr.proto;
					if (nla_put(skb, MDBA_MDB_ENTRY_INFO, sizeof(e), &e)) {
						nla_nest_cancel(skb, nest2);
						err = -EMSGSIZE;
						goto out;
					}
				}
			}
			nla_nest_end(skb, nest2);
		skip:
			idx++;
		}
	}

out:
	cb->args[1] = idx;
	nla_nest_end(skb, nest);
	return err;
}

static int br_mdb_dump(struct sk_buff *skb, struct netlink_callback *cb)
{
	struct net_device *dev;
	struct net *net = sock_net(skb->sk);
	struct nlmsghdr *nlh = NULL;
	int idx = 0, s_idx;

	s_idx = cb->args[0];

	rcu_read_lock();

	/* In theory this could be wrapped to 0... */
	cb->seq = net->dev_base_seq + br_mdb_rehash_seq;

	for_each_netdev_rcu(net, dev) {
		if (dev->priv_flags & IFF_EBRIDGE) {
			struct br_port_msg *bpm;

			if (idx < s_idx)
				goto skip;

			nlh = nlmsg_put(skb, NETLINK_CB(cb->skb).portid,
					cb->nlh->nlmsg_seq, RTM_GETMDB,
					sizeof(*bpm), NLM_F_MULTI);
			if (nlh == NULL)
				break;

			bpm = nlmsg_data(nlh);
			bpm->ifindex = dev->ifindex;
			if (br_mdb_fill_info(skb, cb, dev) < 0)
				goto out;
			if (br_rports_fill_info(skb, cb, dev) < 0)
				goto out;

			cb->args[1] = 0;
			nlmsg_end(skb, nlh);
		skip:
			idx++;
		}
	}

out:
	if (nlh)
		nlmsg_end(skb, nlh);
	rcu_read_unlock();
	cb->args[0] = idx;
	return skb->len;
}

static int nlmsg_populate_mdb_fill(struct sk_buff *skb,
				   struct net_device *dev,
				   struct br_mdb_entry *entry, u32 pid,
				   u32 seq, int type, unsigned int flags)
{
	struct nlmsghdr *nlh;
	struct br_port_msg *bpm;
	struct nlattr *nest, *nest2;

	nlh = nlmsg_put(skb, pid, seq, type, sizeof(*bpm), NLM_F_MULTI);
	if (!nlh)
		return -EMSGSIZE;

	bpm = nlmsg_data(nlh);
	bpm->family  = AF_BRIDGE;
	bpm->ifindex = dev->ifindex;
	nest = nla_nest_start(skb, MDBA_MDB);
	if (nest == NULL)
		goto cancel;
	nest2 = nla_nest_start(skb, MDBA_MDB_ENTRY);
	if (nest2 == NULL)
		goto end;

	if (nla_put(skb, MDBA_MDB_ENTRY_INFO, sizeof(*entry), entry))
		goto end;

	nla_nest_end(skb, nest2);
	nla_nest_end(skb, nest);
	return nlmsg_end(skb, nlh);

end:
	nla_nest_end(skb, nest);
cancel:
	nlmsg_cancel(skb, nlh);
	return -EMSGSIZE;
}

static inline size_t rtnl_mdb_nlmsg_size(void)
{
	return NLMSG_ALIGN(sizeof(struct br_port_msg))
		+ nla_total_size(sizeof(struct br_mdb_entry));
}

static void __br_mdb_notify(struct net_device *dev, struct br_mdb_entry *entry,
			    int type)
{
	struct net *net = dev_net(dev);
	struct sk_buff *skb;
	int err = -ENOBUFS;

	skb = nlmsg_new(rtnl_mdb_nlmsg_size(), GFP_ATOMIC);
	if (!skb)
		goto errout;

	err = nlmsg_populate_mdb_fill(skb, dev, entry, 0, 0, type, NTF_SELF);
	if (err < 0) {
		kfree_skb(skb);
		goto errout;
	}

	rtnl_notify(skb, net, 0, RTNLGRP_MDB, NULL, GFP_ATOMIC);
	return;
errout:
	rtnl_set_sk_err(net, RTNLGRP_MDB, err);
}

void br_mdb_notify(struct net_device *dev, struct net_bridge_port *port,
		   struct br_ip *group, int type)
{
	struct br_mdb_entry entry;

	entry.ifindex = port->dev->ifindex;
	entry.addr.proto = group->proto;
	entry.addr.u.ip4 = group->u.ip4;
#if IS_ENABLED(CONFIG_IPV6)
	entry.addr.u.ip6 = group->u.ip6;
#endif
	__br_mdb_notify(dev, &entry, type);
}

void br_mdb_init(void)
{
	rtnl_register(PF_BRIDGE, RTM_GETMDB, NULL, br_mdb_dump, NULL);
}
