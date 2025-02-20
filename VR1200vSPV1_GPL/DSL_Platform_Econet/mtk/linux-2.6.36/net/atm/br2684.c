/*
 * Ethernet netdevice using ATM AAL5 as underlying carrier
 * (RFC1483 obsoleted by RFC2684) for Linux
 *
 * Authors: Marcell GAL, 2000, XDSL Ltd, Hungary
 *          Eric Kinzie, 2006-2007, US Naval Research Laboratory
 */

#define pr_fmt(fmt) KBUILD_MODNAME ":%s: " fmt, __func__

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/etherdevice.h>
#include <linux/rtnetlink.h>
#include <linux/ip.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <net/arp.h>
#include <linux/atm.h>
#include <linux/atmdev.h>
#include <linux/capability.h>
#include <linux/seq_file.h>

#include <linux/atmbr2684.h>

#include "common.h"

#if !defined(TCSUPPORT_CPU_MT7510) && !defined(TCSUPPORT_CPU_MT7505) && !defined(TCSUPPORT_CPU_EN7512)
#ifdef TCSUPPORT_RA_HWNAT
#include <linux/foe_hook.h>
#endif
#endif



static void skb_debug(const struct sk_buff *skb)
{
#ifdef SKB_DEBUG
#define NUM2PRINT 50
	print_hex_dump(KERN_DEBUG, "br2684: skb: ", DUMP_OFFSET,
		       16, 1, skb->data, min(NUM2PRINT, skb->len), true);
#endif
}

#define BR2684_ETHERTYPE_LEN	2
#define BR2684_PAD_LEN		2

#define LLC		0xaa, 0xaa, 0x03
#define SNAP_BRIDGED	0x00, 0x80, 0xc2
#define SNAP_ROUTED	0x00, 0x00, 0x00
#define PID_ETHERNET	0x00, 0x07
#define ETHERTYPE_IPV4	0x08, 0x00
#define ETHERTYPE_IPV6	0x86, 0xdd
#define PAD_BRIDGED	0x00, 0x00
#define MIN_PKT_SIZE     60
#ifdef CONFIG_SMUX
#if !defined(TCSUPPORT_CT) 
int (*check_smuxIf_exist_hook)(struct net_device *dev);
EXPORT_SYMBOL(check_smuxIf_exist_hook);
#endif
#endif

static const unsigned char ethertype_ipv4[] = { ETHERTYPE_IPV4 };
static const unsigned char ethertype_ipv6[] = { ETHERTYPE_IPV6 };
static const unsigned char llc_oui_pid_pad[] =
			{ LLC, SNAP_BRIDGED, PID_ETHERNET, PAD_BRIDGED };
static const unsigned char llc_oui_ipv4[] = { LLC, SNAP_ROUTED, ETHERTYPE_IPV4 };
static const unsigned char llc_oui_ipv6[] = { LLC, SNAP_ROUTED, ETHERTYPE_IPV6 };

enum br2684_encaps {
	e_vc = BR2684_ENCAPS_VC,
	e_llc = BR2684_ENCAPS_LLC,
};

struct br2684_vcc {
	struct atm_vcc *atmvcc;
	struct net_device *device;
	/* keep old push, pop functions for chaining */
	void (*old_push)(struct atm_vcc *vcc, struct sk_buff *skb);
	void (*old_pop)(struct atm_vcc *vcc, struct sk_buff *skb);
	enum br2684_encaps encaps;
	struct list_head brvccs;
#ifdef CONFIG_ATM_BR2684_IPFILTER
	struct br2684_filter filter;
#endif /* CONFIG_ATM_BR2684_IPFILTER */
	unsigned copies_needed, copies_failed;
};

#ifdef SINGLE_PVC_MULTI_CONNECT
#include "../bridge/br_private.h" /* for __br_fdb_get() */

struct net_devext {
    struct list_head list;
    struct net_device net_dev;
    struct net_device_stats stats;
    enum br2684_payload payload;
};
#endif /* SINGLE_PVC_MULTI_CONNECT */


struct br2684_dev {
	struct net_device *net_dev;
#ifdef SINGLE_PVC_MULTI_CONNECT
	struct list_head net_devexts; /* 横向链表, 相同PVC */
#endif /* SINGLE_PVC_MULTI_CONNECT */
	struct list_head br2684_devs; /* 纵向链表, 不同PVC */
	int number;
	struct list_head brvccs;	/* one device <=> one vcc (before xmas) */
	int mac_was_set;
	enum br2684_payload payload;
};


#if defined(TCSUPPORT_CPU_MT7510) || defined(TCSUPPORT_CPU_MT7505) || defined(TCSUPPORT_CPU_EN7512)
int napi_en = 0;
EXPORT_SYMBOL(napi_en);

void (*br2684_config_hook)(int linkMode, int linkType);
EXPORT_SYMBOL(br2684_config_hook);

int (*br2684_init_hook)(struct atm_vcc *atmvcc, int encaps) = NULL;
EXPORT_SYMBOL(br2684_init_hook);

int (*br2684_push_hook)(struct atm_vcc *atmvcc, struct sk_buff *skb) = NULL;
EXPORT_SYMBOL(br2684_push_hook);

int (*br2684_xmit_hook)(struct sk_buff *skb, struct net_device *dev, struct br2684_vcc *brvcc) = NULL;
EXPORT_SYMBOL(br2684_xmit_hook);
#endif


/*
 * This lock should be held for writing any time the list of devices or
 * their attached vcc's could be altered.  It should be held for reading
 * any time these are being queried.  Note that we sometimes need to
 * do read-locking under interrupt context, so write locking must block
 * the current CPU's interrupts
 */
static DEFINE_RWLOCK(devs_lock);

static LIST_HEAD(br2684_devs);

static inline struct br2684_dev *BRPRIV(const struct net_device *net_dev)
{
#ifdef SINGLE_PVC_MULTI_CONNECT
	return (struct br2684_dev *) net_dev->priv;
#else /* !SINGLE_PVC_MULTI_CONNECT */
	return (struct br2684_dev *)netdev_priv(net_dev);
#endif /* SINGLE_PVC_MULTI_CONNECT */
}

static inline struct net_device *list_entry_brdev(const struct list_head *le)
{
	return list_entry(le, struct br2684_dev, br2684_devs)->net_dev;
}

static inline struct br2684_vcc *BR2684_VCC(const struct atm_vcc *atmvcc)
{
	return (struct br2684_vcc *)(atmvcc->user_back);
}

static inline struct br2684_vcc *list_entry_brvcc(const struct list_head *le)
{
	return list_entry(le, struct br2684_vcc, brvccs);
}

#ifdef SINGLE_PVC_MULTI_CONNECT
static inline struct net_devext *EXTPRIV(const struct net_device *net_dev)
{
	return (struct net_devext *) net_dev->atalk_ptr;
}

static inline struct net_device *list_entry_devext(const struct list_head *le)
{
	return &(list_entry(le, struct net_devext, list)->net_dev);
}
#endif /* SINGLE_PVC_MULTI_CONNECT */

/* Caller should hold read_lock(&devs_lock) */
static struct net_device *br2684_find_dev(const struct br2684_if_spec *s)
{
	struct list_head *lh;
	struct net_device *net_dev;
#ifdef SINGLE_PVC_MULTI_CONNECT
	struct br2684_dev *brdev;
	struct net_device *ext_net_dev;
	struct list_head *lh_dev_ext;
#endif /* SINGLE_PVC_MULTI_CONNECT */
	switch (s->method) {
	case BR2684_FIND_BYNUM:
		list_for_each(lh, &br2684_devs) {
			net_dev = list_entry_brdev(lh);
			if (BRPRIV(net_dev)->number == s->spec.devnum)
				return net_dev;
		}
		break;
	case BR2684_FIND_BYIFNAME:
		list_for_each(lh, &br2684_devs) {
			net_dev = list_entry_brdev(lh);
			if (!strncmp(net_dev->name, s->spec.ifname, IFNAMSIZ))
				return net_dev;
#ifdef SINGLE_PVC_MULTI_CONNECT
			brdev = net_dev->priv;
			if(!list_empty(&(brdev->net_devexts)))
			{
				list_for_each(lh_dev_ext, &(brdev->net_devexts)) 
				{
					ext_net_dev = list_entry_devext(lh_dev_ext);	/* ext node for current PVC list */
					if (!strncmp(ext_net_dev->name, s->spec.ifname, IFNAMSIZ))
						return ext_net_dev;
				}
			}
#endif /* SINGLE_PVC_MULTI_CONNECT */
		}
		break;
	}
	return NULL;
}

static int atm_dev_event(struct notifier_block *this, unsigned long event,
		 void *arg)
{
	struct atm_dev *atm_dev = arg;
	struct list_head *lh;
	struct net_device *net_dev;
	struct br2684_vcc *brvcc;
	struct atm_vcc *atm_vcc;
	unsigned long flags;

	pr_debug("event=%ld dev=%p\n", event, atm_dev);

	read_lock_irqsave(&devs_lock, flags);
	list_for_each(lh, &br2684_devs) {
		net_dev = list_entry_brdev(lh);

		list_for_each_entry(brvcc, &BRPRIV(net_dev)->brvccs, brvccs) {
			atm_vcc = brvcc->atmvcc;
			if (atm_vcc && brvcc->atmvcc->dev == atm_dev) {

				if (atm_vcc->dev->signal == ATM_PHY_SIG_LOST)
					netif_carrier_off(net_dev);
				else
					netif_carrier_on(net_dev);

			}
		}
	}
	read_unlock_irqrestore(&devs_lock, flags);

	return NOTIFY_DONE;
}

static struct notifier_block atm_dev_notifier = {
	.notifier_call = atm_dev_event,
};

#ifdef SINGLE_PVC_MULTI_CONNECT
/* check if net device exist, added by xcl, 20130419 */
static int br2684_net_dev_available(struct atm_vcc *vcc, struct sk_buff *skb)
{
	struct br2684_vcc *brvcc = BR2684_VCC(vcc);
	struct net_device *net_dev = brvcc->device;
	struct br2684_dev *brdev = BRPRIV(net_dev);
	struct list_head *lh;
	struct net_device * net_dev_ext;

	if(skb->dev == net_dev) 
	{
		return 1;
	}

	if (!list_empty(&brdev->net_devexts))
	{
		list_for_each(lh, &brdev->net_devexts)
		{
			net_dev_ext = list_entry_devext(lh);

			if (skb->dev == net_dev_ext)
			{
				return 1;
			}
		}	
	}
	
	return 0;
}
#endif

/* chained vcc->pop function.  Check if we should wake the netif_queue */
static void br2684_pop(struct atm_vcc *vcc, struct sk_buff *skb)
{
	struct br2684_vcc *brvcc = BR2684_VCC(vcc);
	struct net_device *net_dev = skb->dev;

	pr_debug("(vcc %p ; net_dev %p )\n", vcc, net_dev);
	brvcc->old_pop(vcc, skb);

	if (!net_dev)
		return;

#ifdef SINGLE_PVC_MULTI_CONNECT
	/* 解决删除pvc扩展接口时可能导致的kernel panic，原因是扩展接口的net device已被删除，
	 * 但sar中残留的skb->dev还是指向原接口. Added by xcl, 20130419. 
	 */
	if (!br2684_net_dev_available(vcc, skb))
	{
		return;
	}
#endif

	if (atm_may_send(vcc, 0))
		netif_wake_queue(net_dev);

}
/*
 * Send a packet out a particular vcc.  Not to useful right now, but paves
 * the way for multiple vcc's per itf.  Returns true if we can send,
 * otherwise false
 */
static int br2684_xmit_vcc(struct sk_buff *skb, struct net_device *dev,
			   struct br2684_vcc *brvcc)
{
	struct br2684_dev *brdev = BRPRIV(dev);
	struct atm_vcc *atmvcc;
	int minheadroom = (brvcc->encaps == e_llc) ? 10 : 2;
	int err = 0;

	if (skb_headroom(skb) < minheadroom) {
		struct sk_buff *skb2 = skb_realloc_headroom(skb, minheadroom);
		brvcc->copies_needed++;
		dev_kfree_skb(skb);
		if (skb2 == NULL) {
			brvcc->copies_failed++;
			return 0;
		}
		skb = skb2;
	}

#if defined(TCSUPPORT_CPU_MT7510) || defined(TCSUPPORT_CPU_MT7505) || defined(TCSUPPORT_CPU_EN7512)
	if (br2684_xmit_hook){
		err = br2684_xmit_hook(skb, dev, brvcc);
		if (err){
			return 0;
		}
	}
	else 
#endif
	{
	if (brvcc->encaps == e_llc) {
		if (brdev->payload == p_bridged) {
			skb_push(skb, sizeof(llc_oui_pid_pad));
			skb_copy_to_linear_data(skb, llc_oui_pid_pad,
						sizeof(llc_oui_pid_pad));
		} else if (brdev->payload == p_routed) {
			unsigned short prot = ntohs(skb->protocol);

			skb_push(skb, sizeof(llc_oui_ipv4));
			switch (prot) {
			case ETH_P_IP:
				skb_copy_to_linear_data(skb, llc_oui_ipv4,
							sizeof(llc_oui_ipv4));
				break;
			case ETH_P_IPV6:
				skb_copy_to_linear_data(skb, llc_oui_ipv6,
							sizeof(llc_oui_ipv6));
				break;
			default:
				dev_kfree_skb(skb);
				return 0;
			}
		}
	} else { /* e_vc */
		if (brdev->payload == p_bridged) {
			skb_push(skb, 2);
			memset(skb->data, 0, 2);
		} else { /* p_routed */
//			skb_pull(skb, ETH_HLEN); //mark by tangping
		}
	}
	}

	skb_debug(skb);

	ATM_SKB(skb)->vcc = atmvcc = brvcc->atmvcc;
	pr_debug("atm_skb(%p)->vcc(%p)->dev(%p)\n", skb, atmvcc, atmvcc->dev);
	atomic_add(skb->truesize, &sk_atm(atmvcc)->sk_wmem_alloc);
	ATM_SKB(skb)->atm_options = atmvcc->atm_options;
	dev->stats.tx_packets++;
	dev->stats.tx_bytes += skb->len;
	if(atmvcc->send == NULL)
	{
		printk("\r\n[br2684_xmit_vcc]++++atmvcc->send == NULL++++");
		dev_kfree_skb(skb);
		return 0;
	}
	atmvcc->send(atmvcc, skb);

#if !defined(CONFIG_CPU_TC3162) && !defined(CONFIG_MIPS_TC3262)//xflu
	if (!atm_may_send(atmvcc, 0)) {
		netif_stop_queue(brvcc->device);
		/*check for race with br2684_pop*/
		if (atm_may_send(atmvcc, 0))
			netif_start_queue(brvcc->device);
	}

#endif
	return 1;
}

static inline struct br2684_vcc *pick_outgoing_vcc(const struct sk_buff *skb,
						   const struct br2684_dev *brdev)
{
	return list_empty(&brdev->brvccs) ? NULL : list_entry_brvcc(brdev->brvccs.next);	/* 1 vcc/dev right now */
}


#if defined(TCSUPPORT_CPU_MT7510) || defined(TCSUPPORT_CPU_MT7505) || defined(TCSUPPORT_CPU_EN7512)
netdev_tx_t br2684_start_xmit(struct sk_buff *skb,
				     struct net_device *dev)
#else
static netdev_tx_t br2684_start_xmit(struct sk_buff *skb,
				     struct net_device *dev)
#endif
{
	struct br2684_dev *brdev = BRPRIV(dev);
	struct br2684_vcc *brvcc;

	pr_debug("skb_dst(skb)=%p\n", skb_dst(skb));
	read_lock(&devs_lock);
	brvcc = pick_outgoing_vcc(skb, brdev);
	if (brvcc == NULL) {
		pr_debug("no vcc attached to dev %s\n", dev->name);
		dev->stats.tx_errors++;
		dev->stats.tx_carrier_errors++;
		/* netif_stop_queue(dev); */
		dev_kfree_skb(skb);
		read_unlock(&devs_lock);
		return NETDEV_TX_OK;
	}
	/*if the packet length < 60, pad upto 60 bytes. shnwind 2008.4.17*/
        if (skb->len < MIN_PKT_SIZE)
        {
           struct sk_buff *skb2=skb_copy_expand(skb, 0, MIN_PKT_SIZE - skb->len, GFP_ATOMIC);
           dev_kfree_skb(skb);
           if (skb2 == NULL) {
              brvcc->copies_failed++;
	      read_unlock(&devs_lock);
              return NETDEV_TX_OK;
           }
           skb = skb2;		
           memset(skb->tail, 0, MIN_PKT_SIZE - skb->len);		
           skb_put(skb, MIN_PKT_SIZE - skb->len);
        }
	if (!br2684_xmit_vcc(skb, dev, brvcc)) {
		/*
		 * We should probably use netif_*_queue() here, but that
		 * involves added complication.  We need to walk before
		 * we can run.
		 *
		 * Don't free here! this pointer might be no longer valid!
		 */
		dev->stats.tx_errors++;
		dev->stats.tx_fifo_errors++;
	}
	read_unlock(&devs_lock);
	return NETDEV_TX_OK;
}

#if defined(TCSUPPORT_CPU_MT7510) || defined(TCSUPPORT_CPU_MT7505) || defined(TCSUPPORT_CPU_EN7512)
EXPORT_SYMBOL(br2684_start_xmit);
#endif

/*
 * We remember when the MAC gets set, so we don't override it later with
 * the ESI of the ATM card of the first VC
 */
static int br2684_mac_addr(struct net_device *dev, void *p)
{
	int err = eth_mac_addr(dev, p);
	if (!err)
		BRPRIV(dev)->mac_was_set = 1;
	return err;
}

#ifdef CONFIG_ATM_BR2684_IPFILTER
/* this IOCTL is experimental. */
static int br2684_setfilt(struct atm_vcc *atmvcc, void __user * arg)
{
	struct br2684_vcc *brvcc;
	struct br2684_filter_set fs;

	if (copy_from_user(&fs, arg, sizeof fs))
		return -EFAULT;
	if (fs.ifspec.method != BR2684_FIND_BYNOTHING) {
		/*
		 * This is really a per-vcc thing, but we can also search
		 * by device.
		 */
		struct br2684_dev *brdev;
		read_lock(&devs_lock);
		brdev = BRPRIV(br2684_find_dev(&fs.ifspec));
		if (brdev == NULL || list_empty(&brdev->brvccs) ||
		    brdev->brvccs.next != brdev->brvccs.prev)	/* >1 VCC */
			brvcc = NULL;
		else
			brvcc = list_entry_brvcc(brdev->brvccs.next);
		read_unlock(&devs_lock);
		if (brvcc == NULL)
			return -ESRCH;
	} else
		brvcc = BR2684_VCC(atmvcc);
	memcpy(&brvcc->filter, &fs.filter, sizeof(brvcc->filter));
	return 0;
}

/* Returns 1 if packet should be dropped */
static inline int
packet_fails_filter(__be16 type, struct br2684_vcc *brvcc, struct sk_buff *skb)
{
	if (brvcc->filter.netmask == 0)
		return 0;	/* no filter in place */
	if (type == htons(ETH_P_IP) &&
	    (((struct iphdr *)(skb->data))->daddr & brvcc->filter.
	     netmask) == brvcc->filter.prefix)
		return 0;
	if (type == htons(ETH_P_ARP))
		return 0;
	/*
	 * TODO: we should probably filter ARPs too.. don't want to have
	 * them returning values that don't make sense, or is that ok?
	 */
	return 1;		/* drop */
}
#endif /* CONFIG_ATM_BR2684_IPFILTER */

static void br2684_close_vcc(struct br2684_vcc *brvcc)
{
	pr_debug("removing VCC %p from dev %p\n", brvcc, brvcc->device);
	printk("removing VCC %p from dev %p\n", brvcc, brvcc->device);
	write_lock_irq(&devs_lock);
	list_del(&brvcc->brvccs);
	write_unlock_irq(&devs_lock);
	brvcc->atmvcc->user_back = NULL;	/* what about vcc->recvq ??? */
	brvcc->old_push(brvcc->atmvcc, NULL);	/* pass on the bad news */
	kfree(brvcc);
	module_put(THIS_MODULE);
}
#if defined(CONFIG_CPU_TC3162) || defined(CONFIG_MIPS_TC3262)
static void br2684_destroy(struct atm_vcc *atmvcc)
{
	struct br2684_vcc *brvcc = BR2684_VCC(atmvcc);
	struct net_device *net_dev = brvcc->device;
	struct br2684_dev *brdev = BRPRIV(net_dev);
#ifdef CONFIG_SMUX
#if !defined(TCSUPPORT_CT)
	unsigned char ifNum = 0;
#endif
#endif

#ifdef SINGLE_PVC_MULTI_CONNECT
	struct net_devext * net_dev_ext = NULL;
	struct list_head *lh_tmp = NULL;
	struct net_device *ext_net_dev = NULL;
#endif /* SINGLE_PVC_MULTI_CONNECT */

#ifdef CONFIG_SMUX
#if !defined(TCSUPPORT_CT) 
	if(check_smuxIf_exist_hook != NULL) {
		if((ifNum = check_smuxIf_exist_hook(brvcc->device)) > 0) {
			printk("\n==> Exist %d smux interfaces, just return and do not close PVC\n", ifNum);
			return;//If smux interface exist, just return and do not close PVC
		}
	}
#endif
#endif
	br2684_close_vcc(brvcc);
	if (list_empty(&brdev->brvccs)) {
			write_lock_irq(&devs_lock);
			list_del(&brdev->br2684_devs);
			write_unlock_irq(&devs_lock);

#ifdef SINGLE_PVC_MULTI_CONNECT
		if(!list_empty(&(brdev->net_devexts)))
		{
			lh_tmp = brdev->net_devexts.next;
			while( lh_tmp != &(brdev->net_devexts))
			{
				ext_net_dev = list_entry_devext(lh_tmp);
				net_dev_ext = ext_net_dev->atalk_ptr;	

				unregister_netdev(ext_net_dev);
				lh_tmp = lh_tmp->next;
				read_lock(&devs_lock);
				list_del(&net_dev_ext->list);
				read_unlock(&devs_lock);		
				kfree(net_dev_ext);
			}
		}
#endif /* SINGLE_PVC_MULTI_CONNECT */
			
			unregister_netdev(net_dev);
			free_netdev(net_dev);
		}
}
#endif


/* when AAL5 PDU comes in: */
 static void br2684_push(struct atm_vcc *atmvcc, struct sk_buff *skb)
{
	struct br2684_vcc *brvcc = BR2684_VCC(atmvcc);
	struct net_device *net_dev = brvcc->device;
	struct br2684_dev *brdev = BRPRIV(net_dev);
	int err = 0;

#ifdef SINGLE_PVC_MULTI_CONNECT
	struct list_head *lh;
	struct net_device * net_dev_ext;
	struct net_devext *devext;
	struct sk_buff * skb2;
	unsigned char *dstAddr;
#endif /* SINGLE_PVC_MULTI_CONNECT */

	pr_debug("\n");

#if !defined(TCSUPPORT_CPU_MT7510) &&  !defined(TCSUPPORT_CPU_MT7505)  &&  !defined(TCSUPPORT_CPU_EN7512) 
#ifdef TCSUPPORT_RA_HWNAT
	int hwnatFwd = 0;
#endif
#endif


	if (unlikely(skb == NULL)) {
#if defined(CONFIG_CPU_TC3162) || defined(CONFIG_MIPS_TC3262)
		br2684_destroy(atmvcc);
#else
		/* skb==NULL means VCC is being destroyed */
		br2684_close_vcc(brvcc);
		if (list_empty(&brdev->brvccs)) {
			write_lock_irq(&devs_lock);
			list_del(&brdev->br2684_devs);
			write_unlock_irq(&devs_lock);
			unregister_netdev(net_dev);
			free_netdev(net_dev);
		}
#endif
		return;
	}
	skb_debug(skb);
	atm_return(atmvcc, skb->truesize);
	pr_debug("skb from brdev %p\n", brdev);

#if defined(TCSUPPORT_CPU_MT7510) || defined(TCSUPPORT_CPU_MT7505) || defined(TCSUPPORT_CPU_EN7512)
	// hardware handle mpoa header
	if (br2684_push_hook){
		err = br2684_push_hook(atmvcc, skb);
		if (err){
			goto error;
		}	
	} 
	// soft handle mpoa header
	else 
#endif
	{
	if (brvcc->encaps == e_llc) {
	//	if (skb->len > 7 && skb->data[7] == 0x01)
	//		__skb_trim(skb, skb->len - 4);

		/* accept packets that have "ipv[46]" in the snap header */
		if ((skb->len >= (sizeof(llc_oui_ipv4))) &&
		    (memcmp(skb->data, llc_oui_ipv4,
			    sizeof(llc_oui_ipv4) - BR2684_ETHERTYPE_LEN) == 0) &&
 				(brdev->payload == p_routed))//add this line to fix Bug#8296 --Trey
		{
			if (memcmp(skb->data + 6, ethertype_ipv6,
				   sizeof(ethertype_ipv6)) == 0)
				skb->protocol = htons(ETH_P_IPV6);
			else if (memcmp(skb->data + 6, ethertype_ipv4,
					sizeof(ethertype_ipv4)) == 0)
				skb->protocol = htons(ETH_P_IP);
			else
				goto error;
			skb_pull(skb, sizeof(llc_oui_ipv4));
			skb_reset_network_header(skb);
			skb->pkt_type = PACKET_HOST;
		/*
		 * Let us waste some time for checking the encapsulation.
		 * Note, that only 7 char is checked so frames with a valid FCS
		 * are also accepted (but FCS is not checked of course).
		 */
		} else if ((skb->len >= sizeof(llc_oui_pid_pad)) &&
			   (memcmp(skb->data, llc_oui_pid_pad, 7) == 0) &&
				(brdev->payload == p_bridged)) {
				if (skb->data[7] == 0x01)
					__skb_trim(skb, skb->len - 4);
			skb_pull(skb, sizeof(llc_oui_pid_pad));
			skb->protocol = eth_type_trans(skb, net_dev);

				#if !defined(TCSUPPORT_CPU_MT7510) && !defined(TCSUPPORT_CPU_MT7505) && !defined(TCSUPPORT_CPU_EN7512) 
			#ifdef TCSUPPORT_RA_HWNAT
			hwnatFwd = 1;
			#endif
				#endif

			#ifdef TCSUPPORT_BRIDGE_FASTPATH
#if !defined(TCSUPPORT_CT) 
			skb->fb_flags |= FB_WAN_ENABLE;
#endif
			
			#endif
		} else
			goto error;

	} else { /* e_vc */
		if (brdev->payload == p_routed) {
			struct iphdr *iph;

			skb_reset_network_header(skb);
			iph = ip_hdr(skb);
			if (iph->version == 4)
				skb->protocol = htons(ETH_P_IP);
			else if (iph->version == 6)
				skb->protocol = htons(ETH_P_IPV6);
			else
				goto error;
			skb->pkt_type = PACKET_HOST;
		} else { /* p_bridged */
			/* first 2 chars should be 0 */
			if (*((u16 *) (skb->data)) != 0)
				goto error;
			skb_pull(skb, BR2684_PAD_LEN);
			skb->protocol = eth_type_trans(skb, net_dev);

				#if !defined(TCSUPPORT_CPU_MT7510) && !defined(TCSUPPORT_CPU_MT7505) && !defined(TCSUPPORT_CPU_EN7512) 
#ifdef TCSUPPORT_RA_HWNAT
			hwnatFwd = 1;
#endif
				#endif

#ifdef TCSUPPORT_BRIDGE_FASTPATH
#if !defined(TCSUPPORT_CT) 
			skb->fb_flags |= FB_WAN_ENABLE;
#endif
#endif

		}
	}
	}

#ifdef CONFIG_ATM_BR2684_IPFILTER
	if (unlikely(packet_fails_filter(skb->protocol, brvcc, skb)))
		goto dropped;
#endif /* CONFIG_ATM_BR2684_IPFILTER */
	skb->dev = net_dev;
	ATM_SKB(skb)->vcc = atmvcc;	/* needed ? */
	pr_debug("received packet's protocol: %x\n", ntohs(skb->protocol));
	skb_debug(skb);
	/* sigh, interface is down? */
	if (unlikely(!(net_dev->flags & IFF_UP)))
		goto dropped;

#ifdef SINGLE_PVC_MULTI_CONNECT
	dstAddr = skb->mac_header;

	if(list_empty(&brdev->net_devexts)) 
	{
			/* brdev */skb->dev->stats.rx_packets++;
			/* brdev */skb->dev->stats.rx_bytes += skb->len;
		memset(ATM_SKB(skb), 0, sizeof(struct atm_skb_data));

		#if defined(TCSUPPORT_CPU_MT7510)
		if (napi_enable)
		{
			netif_receive_skb(skb);
		} 
		else 
		#endif
		{
			netif_rx(skb);
		}
	}
	else
	{
		if(dstAddr[0] & 1)
		{			
			/* 对每个net_devext都发送一份skb的拷贝 */
			list_for_each(lh, &brdev->net_devexts)
			{
				net_dev_ext = list_entry_devext(lh);
				devext = EXTPRIV(net_dev_ext);
				skb2 = skb_copy(skb, GFP_ATOMIC);
				if (NULL == skb2)
				{
					continue;
				}
				skb2->dev = net_dev_ext;
				skb2->pkt_type = PACKET_HOST;

					devext->stats.rx_packets++;
					devext->stats.rx_bytes += skb2->len;
				memset(ATM_SKB(skb2), 0, sizeof(struct atm_skb_data));
				//printk("zxmdebug 2 ===> broadcast. Ext netdevice: %s\n", (skb2->dev)->name);
				#if defined(TCSUPPORT_CPU_MT7510)
				if (napi_enable)
				{
					netif_receive_skb(skb2);
				} 
				else 
			#endif
				{
					netif_rx(skb2);
				}
			}
				/* brdev */skb->dev->stats.rx_packets++;
				/* brdev */skb->dev->stats.rx_bytes += skb->len;
			memset(ATM_SKB(skb), 0, sizeof(struct atm_skb_data));
			//printk("zxmdebug 2 ===> broadcast. netdevice: %s\n", (skb->dev)->name);
				#if defined(TCSUPPORT_CPU_MT7510)
				if (napi_enable)
				{
					netif_receive_skb(skb);
				} 
				else 
				#endif
				{
					netif_rx(skb);
				}		/* 这里不需要更新skb->dev吗? */
		}
		else
		{			
			struct net_bridge_fdb_entry *fdb = NULL;
			struct net_bridge *br = NULL;
				int isPktTxed = 0;//发给Bridge:1，发给Router:2

			/* 第一个循环，将数据包优先发给路由接口 */
			list_for_each(lh, &brdev->net_devexts)
			{
				net_dev_ext = list_entry_devext(lh);
					//printk("zxmdebug 3 ===> %s\n", net_dev_ext->name);
				devext = EXTPRIV(net_dev_ext);

				/* 匹配主接口或者从接口的mac地址 */
				if(!memcmp(dstAddr, net_dev_ext->dev_addr, ETH_ALEN)) 
				{
					skb->dev = net_dev_ext;
					skb->pkt_type = PACKET_HOST;
						/* brdev */skb->dev->stats.rx_packets++;
						/* brdev */skb->dev->stats.rx_bytes += skb->len;
					memset(ATM_SKB(skb), 0, sizeof(struct atm_skb_data));
					//printk("zxmdebug 5 ===> mac equal. netdevice: %s\n", (skb->dev)->name);
					#if defined(TCSUPPORT_CPU_MT7510)
					if (napi_enable)
					{
						netif_receive_skb(skb);
					} 
					else 
					#endif
					{
					netif_rx(skb);
					}
					isPktTxed = 2;
					break;
				}
			}

				/* 没有匹配到路由接口，匹配桥接口 */
			if (0 == isPktTxed)
			{
				list_for_each(lh, &brdev->net_devexts)
				{
					net_dev_ext = list_entry_devext(lh);
					//printk("zxmdebug 6 ===> %s\n", net_dev_ext->name);
					devext = EXTPRIV(net_dev_ext);
					
					if (br_port_exists(net_dev_ext))
					{
						//printk("zxmdebug 3 ===> %s\n", br->dev->name);
						fdb = __br_fdb_get(br_port_get_rcu(net_dev_ext)->br, dstAddr); 
						if (fdb)
						{
							skb->dev = net_dev_ext;
							skb->pkt_type = PACKET_HOST;
							/* brdev */skb->dev->stats.rx_packets++;
							/* brdev */skb->dev->stats.rx_bytes += skb->len;
							memset(ATM_SKB(skb), 0, sizeof(struct atm_skb_data));
							//printk("zxmdebug 5 ===> mac equal. netdevice: %s\n", (skb->dev)->name);
							#if defined(TCSUPPORT_CPU_MT7510)
							if (napi_enable)
							{
								netif_receive_skb(skb);
							} 
							else 
							#endif
							{
								netif_rx(skb);
							}
							isPktTxed = 1;
							break;
						}
					}						
				}
			}

				
				/* wzy: 如果即没匹配到Router又没匹配到Bridge的话, 需要洪泛到所有扩展接口; 17Apr12 */
			if (0 == isPktTxed)
			{
				// 对每个net_devext都发送一份skb的拷贝 
				list_for_each(lh, &brdev->net_devexts)
				{
					net_dev_ext = list_entry_devext(lh);
					devext = EXTPRIV(net_dev_ext);
					skb2 = skb_copy(skb, GFP_ATOMIC);
					if (NULL == skb2)
					{
						continue;
					}
					skb2->dev = net_dev_ext;
					skb2->pkt_type = PACKET_HOST;

					devext->stats.rx_packets++;
					devext->stats.rx_bytes += skb2->len;
					memset(ATM_SKB(skb2), 0, sizeof(struct atm_skb_data));
					//printk("zxmdebug 6 ===> other. Ext netdevice: %s\n", (skb2->dev)->name);
					#if defined(TCSUPPORT_CPU_MT7510)
					if (napi_enable)
					{
						netif_receive_skb(skb2);
					} 
					else 
					#endif
					{
						netif_rx(skb2);
					}
				}
					/* brdev */skb->dev->stats.rx_packets++;
					/* brdev */skb->dev->stats.rx_bytes += skb->len;
				memset(ATM_SKB(skb), 0, sizeof(struct atm_skb_data));
				//printk("zxmdebug 6 ===> other.. netdevice: %s\n", (skb->dev)->name);
				#if defined(TCSUPPORT_CPU_MT7510)
				if (napi_enable)
				{
					netif_receive_skb(skb);
				} 
				else 
				#endif
				{
					netif_rx(skb);
				}
				isPktTxed = 2;
			}

#if 0	/* wzy: modify at 17Apr12 */
				if (isPktTxed != 2)
			{
				dev_kfree_skb(skb);
				//printk("zxmdebug 7 === > drop!\n");
					brdev->stats.rx_dropped++;
				}
#else
				if (isPktTxed == 0)
				{
					goto dropped;
			}
#endif /* #if 0 */
		}
	}	
#else /* !SINGLE_PVE_MULTI_CONNECT */
	net_dev->stats.rx_packets++;
	net_dev->stats.rx_bytes += skb->len;
	memset(ATM_SKB(skb), 0, sizeof(struct atm_skb_data));

	#if !defined(TCSUPPORT_CPU_MT7510) && !defined(TCSUPPORT_CPU_MT7505) && !defined(TCSUPPORT_CPU_EN7512) 
#ifdef TCSUPPORT_RA_HWNAT
	if (hwnatFwd) {
		if (ra_sw_nat_hook_set_magic)  
			ra_sw_nat_hook_set_magic(skb, FOE_MAGIC_ATM);

		if (ra_sw_nat_hook_rx != NULL) {
			if (ra_sw_nat_hook_rx(skb) == 0) 
				return;
		}
	}
#endif
	#endif


#if defined(TCSUPPORT_CPU_MT7510) || defined(TCSUPPORT_CPU_MT7505) || defined(TCSUPPORT_CPU_EN7512) 
	if (napi_en)
	{
		netif_receive_skb(skb);
	} 
	else 
#endif
	{
	netif_rx(skb);
	}
#endif /* SINGLE_PVC_MULTI_CONNECT */
	return;

dropped:
	net_dev->stats.rx_dropped++;
	goto free_skb;
error:
	net_dev->stats.rx_errors++;
free_skb:
	dev_kfree_skb(skb);
}

/*
 * Assign a vcc to a dev
 * Note: we do not have explicit unassign, but look at _push()
 */
static int br2684_regvcc(struct atm_vcc *atmvcc, void __user * arg)
{
	struct sk_buff_head queue;
	int err;
	struct br2684_vcc *brvcc;
	struct sk_buff *skb, *tmp;
	struct sk_buff_head *rq;
	struct br2684_dev *brdev;
	struct net_device *net_dev;
	struct atm_backend_br2684 be;
	unsigned long flags;

	if (copy_from_user(&be, arg, sizeof be))
		return -EFAULT;

#if defined(TCSUPPORT_CPU_MT7510) || defined(TCSUPPORT_CPU_MT7505) || defined(TCSUPPORT_CPU_EN7512) 
	if (br2684_init_hook){
		printk("enter br2684_init_hook function\n");
		err = br2684_init_hook(atmvcc, be.encaps);
		if (err){
			printk("br2684_init_hook: error detected\n");
			return err;
		} else {
			printk("br2684_init_hook: success\n");
		}
	} else {
		printk("br2684_init_hook function: (NULL)\n");
	}
#endif

	brvcc = kzalloc(sizeof(struct br2684_vcc), GFP_KERNEL);
	if (!brvcc)
		return -ENOMEM;
	write_lock_irq(&devs_lock);
	net_dev = br2684_find_dev(&be.ifspec);
	if (net_dev == NULL) {
		pr_err("tried to attach to non-existant device\n");
		err = -ENXIO;
		goto error;
	}
#ifdef CONFIG_NET_SCHED  /*Rodney_20091115*/
	atmvcc->_dev = net_dev;
#endif	
	brdev = BRPRIV(net_dev);
	if (atmvcc->push == NULL) {
		err = -EBADFD;
		goto error;
	}
	if (!list_empty(&brdev->brvccs)) {
		/* Only 1 VCC/dev right now */
		err = -EEXIST;
		goto error;
	}
	if (be.fcs_in != BR2684_FCSIN_NO ||
	    be.fcs_out != BR2684_FCSOUT_NO ||
	    be.fcs_auto || be.has_vpiid || be.send_padding ||
	    (be.encaps != BR2684_ENCAPS_VC &&
	     be.encaps != BR2684_ENCAPS_LLC) ||
	    be.min_size != 0) {
		err = -EINVAL;
		goto error;
	}
	pr_debug("vcc=%p, encaps=%d, brvcc=%p\n", atmvcc, be.encaps, brvcc);
	if (list_empty(&brdev->brvccs) && !brdev->mac_was_set) {
		unsigned char *esi = atmvcc->dev->esi;
		if (esi[0] | esi[1] | esi[2] | esi[3] | esi[4] | esi[5])
			memcpy(net_dev->dev_addr, esi, net_dev->addr_len);
		else
			net_dev->dev_addr[2] = 1;
	}
	list_add(&brvcc->brvccs, &brdev->brvccs);
	/* write_unlock_irq(&devs_lock); zl del 2012-6-14 */
	brvcc->device = net_dev;
	brvcc->atmvcc = atmvcc;
	write_unlock_irq(&devs_lock); /* zl add 2012-6-14 */
	atmvcc->user_back = brvcc;
	brvcc->encaps = (enum br2684_encaps)be.encaps;
	brvcc->old_push = atmvcc->push;
	brvcc->old_pop = atmvcc->pop;
	barrier();
	atmvcc->push = br2684_push;
	atmvcc->pop = br2684_pop;

	__skb_queue_head_init(&queue);
	rq = &sk_atm(atmvcc)->sk_receive_queue;

	spin_lock_irqsave(&rq->lock, flags);
	skb_queue_splice_init(rq, &queue);
	spin_unlock_irqrestore(&rq->lock, flags);

	skb_queue_walk_safe(&queue, skb, tmp) {
		struct net_device *dev = skb->dev;
		if(dev != NULL){
			dev->stats.rx_bytes -= skb->len;
			dev->stats.rx_packets--;
		}
		br2684_push(atmvcc, skb);
	}
	/* initialize netdev carrier state */
	if (atmvcc->dev->signal == ATM_PHY_SIG_LOST)
		netif_carrier_off(net_dev);
	else
		netif_carrier_on(net_dev);

	__module_get(THIS_MODULE);
	return 0;

error:
	write_unlock_irq(&devs_lock);
	kfree(brvcc);
	return err;
}

static const struct net_device_ops br2684_netdev_ops = {
	.ndo_start_xmit 	= br2684_start_xmit,
	.ndo_set_mac_address	= br2684_mac_addr,
	.ndo_change_mtu		= eth_change_mtu,
	.ndo_validate_addr	= eth_validate_addr,
};

static const struct net_device_ops br2684_netdev_ops_routed = {
	.ndo_start_xmit 	= br2684_start_xmit,
	.ndo_set_mac_address	= br2684_mac_addr,
	.ndo_change_mtu		= eth_change_mtu
};

static int br2684_unregvcc(struct atm_vcc *atmvcc, void __user *arg)
{
	int err;
	struct br2684_vcc *brvcc;
	struct br2684_dev *brdev;
	struct net_device *net_dev;
	struct atm_backend_br2684 be;

	if (copy_from_user(&be, arg, sizeof be))
		return -EFAULT;
	write_lock_irq(&devs_lock);
	net_dev = br2684_find_dev(&be.ifspec);
	if (net_dev == NULL) {
		printk(KERN_ERR
			"br2684: tried to unregister to non-existant device\n");
		err = -ENXIO;
		goto error;
	}

	brdev = BRPRIV(net_dev);
#ifdef SINGLE_PVC_MULTI_CONNECT
	/* wzy: 如果还存在有扩展接口, 则直接返回error(需要先将全部扩展接口删除再删主接口), 02May12 */
	if (!list_empty(&brdev->net_devexts))
	{
		printk(KERN_ERR
			"br2684: All ext netdev should be deleted before deleting the major node\n");
		err = -ENXIO;
		goto error;
	}
	printk("br2684_unregvcc(): list_empty(&brdev->net_devexts) == TRUE\n");
#endif /* #if 0 */
	
	while (!list_empty(&brdev->brvccs)) {
		brvcc = list_entry_brvcc(brdev->brvccs.next);
		br2684_close_vcc(brvcc);
	}

	list_del(&brdev->br2684_devs);
	write_unlock_irq(&devs_lock);
	unregister_netdev(net_dev);
	free_netdev(net_dev);
	atmvcc->push = NULL;
	vcc_release_async(atmvcc, -ETIMEDOUT);
	return 0;
error:
	write_unlock_irq(&devs_lock);
	return err;
}

static void br2684_setup(struct net_device *netdev)
{
	struct br2684_dev *brdev;

#ifdef SINGLE_PVC_MULTI_CONNECT
	netdev->priv = netdev_priv(netdev);
#endif
	brdev = BRPRIV(netdev);

	ether_setup(netdev);
	brdev->net_dev = netdev;

	netdev->netdev_ops = &br2684_netdev_ops;

	//for vlan tag 0
	netdev->priv_flags |= IFF_802_1Q_DUAL_VLAN;
	
	INIT_LIST_HEAD(&brdev->brvccs);
}

static void br2684_setup_routed(struct net_device *netdev)
{
	struct br2684_dev *brdev;

#ifdef SINGLE_PVC_MULTI_CONNECT
	netdev->priv = netdev_priv(netdev);
#endif
	brdev = BRPRIV(netdev);

	brdev->net_dev = netdev;
	netdev->hard_header_len = 0;
	netdev->netdev_ops = &br2684_netdev_ops_routed;
	netdev->addr_len = 0;
	netdev->mtu = 1500;
	netdev->type = ARPHRD_PPP;
	//netdev->flags = IFF_POINTOPOINT | IFF_NOARP | IFF_MULTICAST;
	netdev->flags = IFF_NOARP | IFF_MULTICAST;
	netdev->tx_queue_len = 100;
	netdev->priv_flags |= IFF_802_1Q_DUAL_VLAN;
	INIT_LIST_HEAD(&brdev->brvccs);
}

static int br2684_create(void __user *arg)
{
	int err;
	struct net_device *netdev;
	struct br2684_dev *brdev;
	struct atm_newif_br2684 ni;
	enum br2684_payload payload;

	pr_debug("\n");

	if (copy_from_user(&ni, arg, sizeof ni))
		return -EFAULT;

	if (ni.media & BR2684_FLAG_ROUTED)
		payload = p_routed;
	else
		payload = p_bridged;
	ni.media &= 0xffff;	/* strip flags */

	if (ni.media != BR2684_MEDIA_ETHERNET || ni.mtu != 1500)
		return -EINVAL;

	printk("br2684_create():create br2684 interface %s with payload:%d\n", 
		ni.ifname[0] ? ni.ifname : "nas%d", payload);
	netdev = alloc_netdev(sizeof(struct br2684_dev),
			      ni.ifname[0] ? ni.ifname : "nas%d",
			      (payload == p_routed) ?
			      br2684_setup_routed : br2684_setup);
	if (!netdev)
		return -ENOMEM;

	/* added by yangxv for QoS */
	netdev->priv_flags |= IFF_WAN_DEV;
	/* end added */

	brdev = BRPRIV(netdev);

#ifdef SINGLE_PVC_MULTI_CONNECT
	brdev->net_dev->priv = brdev;
	INIT_LIST_HEAD(&brdev->net_devexts);
#endif /* SINGLE_PVC_MULTI_CONNECT */

	pr_debug("registered netdev %s\n", netdev->name);
	/* open, stop, do_ioctl ? */
	err = register_netdev(netdev);
	if (err < 0) {
		pr_err("register_netdev failed\n");
		free_netdev(netdev);
		return err;
	}

	write_lock_irq(&devs_lock);

	brdev->payload = payload;

#if defined(TCSUPPORT_CPU_MT7510) || defined(TCSUPPORT_CPU_MT7505) || defined(TCSUPPORT_CPU_EN7512) 
	if (br2684_config_hook){
		br2684_config_hook(payload, 0);
	} else {
		printk("br2684_config_hook function: (NULL)\n");
	}
#endif

	if (list_empty(&br2684_devs)) {
		/* 1st br2684 device */
		brdev->number = 1;
	} else
		brdev->number = BRPRIV(list_entry_brdev(br2684_devs.prev))->number + 1;

	list_add_tail(&brdev->br2684_devs, &br2684_devs);
	write_unlock_irq(&devs_lock);
	return 0;
}

#ifdef SINGLE_PVC_MULTI_CONNECT
extern struct net_device *init_extnetdev_mq(struct net_device *dev, void (*setup)(struct net_device *), 
									unsigned int queue_count);
extern void free_extnetdev(struct net_device *dev);

static void extdev_setup_bridged(struct net_device *netdev)
{
	netdev->type		= ARPHRD_ETHER;
	netdev->hard_header_len 	= ETH_HLEN;
	netdev->mtu		= ETH_DATA_LEN;
	netdev->addr_len		= ETH_ALEN;
	netdev->tx_queue_len	= 1000;	/* Ethernet wants good queues */
	netdev->flags		= IFF_BROADCAST|IFF_MULTICAST;
	netdev->netdev_ops = &br2684_netdev_ops;
	
	netdev->header_ops		= &eth_header_ops;
	memset(netdev->broadcast, 0xFF, ETH_ALEN);
}

static void extdev_setup_routed(struct net_device *netdev)
{
	netdev->type = ARPHRD_PPP;
	netdev->hard_header_len = 0;
	netdev->mtu = 1500;
	netdev->addr_len = 0;
	netdev->tx_queue_len = 100;
	netdev->flags = IFF_POINTOPOINT | IFF_NOARP | IFF_MULTICAST;
	netdev->netdev_ops = &br2684_netdev_ops_routed;
}
static int br2684_ext(void __user *arg)
{
	#if 1
	int err;
	struct net_device *major_netdev;
	struct br2684_dev *brdev;
	struct atm_newif_br2684 ni;
	enum br2684_payload payload;
	
	struct net_devext * net_dev_ext;
	struct br2684_if_spec s;
	int ext;

	pr_debug("br2684_ext\n");

	if (copy_from_user(&ni, arg, sizeof ni)) {
		return -EFAULT;
	}

	if (ni.media & BR2684_FLAG_ROUTED)
		payload = p_routed;
	else
		payload = p_bridged;
	
	ext = (ni.media >> 24) & 0xff;			/* get ext_inf_num */
	ni.media &= 0xffff; /* strip flags */

	if (ni.media != BR2684_MEDIA_ETHERNET || ni.mtu != 1500) {
		return -EINVAL;
	}

	/* 1. 为net_devext申请空间 */
	if ((net_dev_ext = kmalloc(sizeof(struct net_devext), GFP_KERNEL)) == NULL)
		return -ENOMEM;

	/* 2. 初始化net_devext */
	memset(net_dev_ext, 0, sizeof(struct net_devext));

	/* 2.1 初始化net_devext->net_dev */
	if (init_extnetdev_mq(&net_dev_ext->net_dev,
			      		(payload == p_routed) ? extdev_setup_routed : extdev_setup_bridged, 
			      		1) == NULL)
		return -ENOMEM;
	
	s.method = BR2684_FIND_BYIFNAME;
	sprintf(s.spec.ifname, "%s%s", "", ni.ifname); // To pass the link error;
	major_netdev = br2684_find_dev(&s); /* get major_node */
	if (!major_netdev)
	{
		printk(KERN_ERR
		    "br2684: tried to attach to ext_dev to a non-existant device\n");
		return -ENOMEM;
	}
	brdev = BRPRIV(major_netdev);	
	sprintf(net_dev_ext->net_dev.name, "%s_%d", brdev->net_dev->name, ext);
	net_dev_ext->net_dev.atalk_ptr = net_dev_ext;
	
	/* 2.2 初始化扩展节点的net_dev.priv */
	net_dev_ext->net_dev.priv = brdev;
	
	/* 3. 注册net_devext */
	pr_debug("registered netdev %s\n", net_dev_ext->net_dev.name);
	err = register_netdev(&net_dev_ext->net_dev);
	if (err < 0) {
		printk(KERN_ERR "br2684_create: register_netdev failed\n");
		free_netdev(&net_dev_ext->net_dev);
		return err;
	}
	/* Mark br2684 device for QoS */
	net_dev_ext->net_dev.priv_flags |= IFF_WAN_DEV;

	net_dev_ext->net_dev.priv_flags |= IFF_802_1Q_DUAL_VLAN;

	/* 4. 添加到brdev->net_devexts */
	write_lock_irq(&devs_lock);
	list_add_tail(&net_dev_ext->list, &brdev->net_devexts);
	write_unlock_irq(&devs_lock);
	#endif
	return 0;
}


static int br2684_del(void __user *arg)
{
	int del;

	struct atm_newif_br2684 ni;
	struct net_device *net_dev;
	struct net_devext * net_dev_ext;
	struct br2684_if_spec s;

	pr_debug("br2684_del\n");

	if (copy_from_user(&ni, (void *) arg, sizeof ni)) {
		return -EFAULT;
	}


	del = (ni.media >> 24) & 0xff;
	ni.media &= 0xffff; /* strip flags */
	if (ni.media != BR2684_MEDIA_ETHERNET || ni.mtu != 1500) {
		return -EINVAL;
	}
	
	s.method = BR2684_FIND_BYIFNAME;
	if (del > 0)
	{
		sprintf(s.spec.ifname, "%s_%d", ni.ifname, del);
	}
	else
	{
		printk(KERN_ERR "br2684_del: delete nas device error del = %d\n", del);
		return -ENXIO;
	}
	pr_debug("delete netdevice:%s\n\n", s.spec.ifname);
	
	net_dev = br2684_find_dev(&s);
	if (net_dev == NULL) {
		printk(KERN_ERR
		    "br2684: tried to attach to non-existant device\n");
		return -ENXIO;
	}
	
	net_dev_ext = net_dev->atalk_ptr;	
	read_lock(&devs_lock);
	list_del(&net_dev_ext->list);
	read_unlock(&devs_lock);
	unregister_netdev(net_dev);
	free_extnetdev(net_dev);	
	kfree(net_dev_ext);

	return 0;
}

#if 0
/*print the interface name for each devs in list br2684_devs and list net_devexts*/
static void br2684_print_dev(void)
{
	struct list_head *lh_br2684_dev;
	struct list_head *lh_dev_ext;
	struct net_device *net_dev;
	struct br2684_dev *cur_br2684_dev;
	struct net_devext *cur_dev_ext;
	int i;

	i				= 1;
	cur_br2684_dev	= NULL;
	cur_dev_ext		= NULL;
	lh_br2684_dev	= NULL;
	lh_dev_ext		= NULL;

	
	printk("%s  %s           %s\n", "index", "if_name", "if_mac");
	list_for_each(lh_br2684_dev, &br2684_devs) 
	{
		net_dev = list_entry_brdev(lh_br2684_dev);
		cur_br2684_dev = BRPRIV(net_dev);
		if(list_empty(&(cur_br2684_dev->net_devexts)))/* is there any extend if */
		{
			short vpi = 0;
			int vci = 0;
			struct br2684_vcc *vcc = pick_outgoing_vcc(NULL, cur_br2684_dev);
			if (vcc)
			{
				vpi = vcc->atmvcc->vpi;
				vci = vcc->atmvcc->vci;
			}

			printk("%d      %s       %02x:%02x:%02x:%02x:%02x:%02x    (%d/%d)\n", 
				i, net_dev->name, 
				(net_dev->dev_addr)[0], /* big endin */
				(net_dev->dev_addr)[1], 
				(net_dev->dev_addr)[2], 
				(net_dev->dev_addr)[3], 
				(net_dev->dev_addr)[4], 
				(net_dev->dev_addr)[5],
				vpi, vci);
			i++;	
		}
		else
		{
			short vpi = 0;
			int vci = 0;
			struct br2684_vcc *vcc = pick_outgoing_vcc(NULL, cur_br2684_dev);
			if (vcc)
			{
				vpi = vcc->atmvcc->vpi;
				vci = vcc->atmvcc->vci;
			}

			printk("%d      %s       %02x:%02x:%02x:%02x:%02x:%02x    (%d/%d)\n", 
				i, net_dev->name, 
				(net_dev->dev_addr)[0], /* big endin */
				(net_dev->dev_addr)[1], 
				(net_dev->dev_addr)[2], 
				(net_dev->dev_addr)[3], 
				(net_dev->dev_addr)[4], 
				(net_dev->dev_addr)[5],
				vpi, vci);
			i++;
			list_for_each(lh_dev_ext, &(cur_br2684_dev->net_devexts)) 
			{
				net_dev = list_entry_devext(lh_dev_ext);
				printk("  %d    %s     %02x:%02x:%02x:%02x:%02x:%02x\n", 
					i, net_dev->name, 
					(net_dev->dev_addr)[0], 
					(net_dev->dev_addr)[1], 
					(net_dev->dev_addr)[2], 
					(net_dev->dev_addr)[3], 
					(net_dev->dev_addr)[4], 
					(net_dev->dev_addr)[5]);
				i++;
			}
		}
	}
	printk("\n");
}

static void printSkbuff(struct sk_buff *skb)
{
	{
		unsigned char * pdata = skb->data - ETH_HLEN;
		int i = 1;
		
		printk("\nzxmdebug br2684 skbuf data:\n");
		while(pdata != skb->tail)
		{
			printk("%02x ", *pdata);
			if (i%16 == 0)
			{
				printk("\n");
			}
			i++;
			pdata++;
		}
		printk("\n");
	}
}
#endif
#endif /* SINGLE_PVC_MULTI_CONNECT */


/*
 * This handles ioctls actually performed on our vcc - we must return
 * -ENOIOCTLCMD for any unrecognized ioctl
 */
static int br2684_ioctl(struct socket *sock, unsigned int cmd,
			unsigned long arg)
{
	struct atm_vcc *atmvcc = ATM_SD(sock);
	void __user *argp = (void __user *)arg;
	atm_backend_t b;
	int err;
	switch (cmd) {
	case ATM_SETBACKEND:
	case ATM_NEWBACKENDIF:
#ifdef SINGLE_PVC_MULTI_CONNECT
	case ATM_DELBACKENDIF:
	case ATM_ADDEXTBACKENDIF:
	case ATM_DELEXTBACKENDIF:
	case ATM_ATTACHBACKEND:
#endif /* SINGLE_PVC_MULTI_CONNECT */
		err = get_user(b, (atm_backend_t __user *) argp);
		if (err)
			return -EFAULT;
		if (b != ATM_BACKEND_BR2684)
			return -ENOIOCTLCMD;
		if (!capable(CAP_NET_ADMIN))
			return -EPERM;
#ifdef SINGLE_PVC_MULTI_CONNECT
		if (cmd == ATM_SETBACKEND)
		{
			/*printk("br2684_ioctl cmd: ATM_SETBACKEND\n");*/
			return br2684_regvcc(atmvcc, argp);
		}
		else if (cmd == ATM_NEWBACKENDIF)
		{
			/*printk("br2684_ioctl cmd: ATM_NEWBACKENDIF\n");*/
			return br2684_create(argp);
		}
		else if (cmd == ATM_DELBACKENDIF)
		{
			/*printk("br2684_ioctl cmd: ATM_DELBACKENDIF\n");*/
			return br2684_unregvcc(atmvcc, argp);
		}
		else if (cmd == ATM_ADDEXTBACKENDIF)
		{
			/*printk("br2684_ioctl cmd: ATM_ADDEXTBACKENDIF\n");*/
			return br2684_ext(argp);
		}
		else if (cmd == ATM_DELEXTBACKENDIF)		
		{
			/*printk("br2684_ioctl cmd: ATM_DELEXTBACKENDIF\n");*/
			return br2684_del(argp);
		}
		else
		{
			printk("br2684_ioctl error cmd\n");
			//return br2684_ext_attachvcc(atmvcc, argp);
			
		}
#else		
		if (cmd == ATM_SETBACKEND)
			return br2684_regvcc(atmvcc, argp);
		else
			return br2684_create(argp);
#endif /* SINGLE_PVC_MULTI_CONNECT */		
#ifdef CONFIG_ATM_BR2684_IPFILTER
	case BR2684_SETFILT:
		if (atmvcc->push != br2684_push)
			return -ENOIOCTLCMD;
		if (!capable(CAP_NET_ADMIN))
			return -EPERM;
		err = br2684_setfilt(atmvcc, argp);

		return err;
#endif /* CONFIG_ATM_BR2684_IPFILTER */
	}
	return -ENOIOCTLCMD;
}

static struct atm_ioctl br2684_ioctl_ops = {
	.owner = THIS_MODULE,
	.ioctl = br2684_ioctl,
};

#ifdef CONFIG_PROC_FS
static void *br2684_seq_start(struct seq_file *seq, loff_t * pos)
	__acquires(devs_lock)
{
	read_lock(&devs_lock);
	return seq_list_start(&br2684_devs, *pos);
}

static void *br2684_seq_next(struct seq_file *seq, void *v, loff_t * pos)
{
	return seq_list_next(v, &br2684_devs, pos);
}

static void br2684_seq_stop(struct seq_file *seq, void *v)
	__releases(devs_lock)
{
	read_unlock(&devs_lock);
}

static int br2684_seq_show(struct seq_file *seq, void *v)
{
	const struct br2684_dev *brdev = list_entry(v, struct br2684_dev,
						    br2684_devs);
	const struct net_device *net_dev = brdev->net_dev;
	const struct br2684_vcc *brvcc;

	seq_printf(seq, "dev %.16s: num=%d, mac=%pM (%s)\n",
		   net_dev->name,
		   brdev->number,
		   net_dev->dev_addr,
		   brdev->mac_was_set ? "set" : "auto");

	list_for_each_entry(brvcc, &brdev->brvccs, brvccs) {
		seq_printf(seq, "  vcc %d.%d.%d: encaps=%s payload=%s"
			   ", failed copies %u/%u"
			   "\n", brvcc->atmvcc->dev->number,
			   brvcc->atmvcc->vpi, brvcc->atmvcc->vci,
			   (brvcc->encaps == e_llc) ? "LLC" : "VC",
			   (brdev->payload == p_bridged) ? "bridged" : "routed",
			   brvcc->copies_failed, brvcc->copies_needed);
#ifdef CONFIG_ATM_BR2684_IPFILTER
#define b1(var, byte)	((u8 *) &brvcc->filter.var)[byte]
#define bs(var)		b1(var, 0), b1(var, 1), b1(var, 2), b1(var, 3)
		if (brvcc->filter.netmask != 0)
			seq_printf(seq, "    filter=%d.%d.%d.%d/"
				   "%d.%d.%d.%d\n", bs(prefix), bs(netmask));
#undef bs
#undef b1
#endif /* CONFIG_ATM_BR2684_IPFILTER */
	}
	return 0;
}

static const struct seq_operations br2684_seq_ops = {
	.start = br2684_seq_start,
	.next = br2684_seq_next,
	.stop = br2684_seq_stop,
	.show = br2684_seq_show,
};

static int br2684_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &br2684_seq_ops);
}

static const struct file_operations br2684_proc_ops = {
	.owner = THIS_MODULE,
	.open = br2684_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

extern struct proc_dir_entry *atm_proc_root;	/* from proc.c */
#endif /* CONFIG_PROC_FS */

#if defined(TCSUPPORT_CPU_MT7510) || defined(TCSUPPORT_CPU_MT7505) || defined(TCSUPPORT_CPU_EN7512) 
int br2684_init(void)
#else
static int __init br2684_init(void)
#endif
{
#ifdef CONFIG_PROC_FS
	struct proc_dir_entry *p;
	p = proc_create("br2684", 0, atm_proc_root, &br2684_proc_ops);
	if (p == NULL)
		return -ENOMEM;
#endif
	register_atm_ioctl(&br2684_ioctl_ops);
	register_atmdevice_notifier(&atm_dev_notifier);
	return 0;
}


#if defined(TCSUPPORT_CPU_MT7510) || defined(TCSUPPORT_CPU_MT7505) || defined(TCSUPPORT_CPU_EN7512) 
void br2684_exit(void)
#else
static void __exit br2684_exit(void)
#endif
{
	struct net_device *net_dev;
	struct br2684_dev *brdev;
	struct br2684_vcc *brvcc;
	deregister_atm_ioctl(&br2684_ioctl_ops);

#ifdef CONFIG_PROC_FS
	remove_proc_entry("br2684", atm_proc_root);
#endif


	unregister_atmdevice_notifier(&atm_dev_notifier);

	while (!list_empty(&br2684_devs)) {
		net_dev = list_entry_brdev(br2684_devs.next);
		brdev = BRPRIV(net_dev);
		while (!list_empty(&brdev->brvccs)) {
			brvcc = list_entry_brvcc(brdev->brvccs.next);
			br2684_close_vcc(brvcc);
		}

		list_del(&brdev->br2684_devs);
		unregister_netdev(net_dev);
		free_netdev(net_dev);
	}
}


#if defined(TCSUPPORT_CPU_MT7510) || defined(TCSUPPORT_CPU_MT7505) || defined(TCSUPPORT_CPU_EN7512) 
EXPORT_SYMBOL(br2684_init);
EXPORT_SYMBOL(br2684_exit);
#endif


#if !defined(TCSUPPORT_CPU_MT7510) && !defined(TCSUPPORT_CPU_MT7505) && !defined(TCSUPPORT_CPU_EN7512) 
module_init(br2684_init);
module_exit(br2684_exit);
#endif


MODULE_AUTHOR("Marcell GAL");
MODULE_DESCRIPTION("RFC2684 bridged protocols over ATM/AAL5");
MODULE_LICENSE("GPL");
