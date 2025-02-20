/*
 *	Handle incoming frames
 *	Linux ethernet bridge
 *
 *	Authors:
 *	Lennert Buytenhek		<buytenh@gnu.org>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/netfilter_bridge.h>
#include "br_private.h"
#ifdef CONFIG_TP_IGMP_SNOOPING
#include "br_igmp.h"
#endif

#ifdef TCSUPPORT_INIC_HOST
#include <linux/mtd/fttdp_inic.h>
#endif

#if !defined(TCSUPPORT_CT) 
#ifdef CONFIG_PORT_BINDING
#if defined(TCSUPPORT_ROUTEPOLICY_PRIOR_PORTBIND)
extern int (*portbind_sw_prior_hook)(struct sk_buff *skb);
#endif
extern int (*portbind_sw_hook)(void);
extern int (*portbind_check_hook)(char *inIf, char *outIf);
#endif
#endif
#if defined(TCSUPPORT_ROUTEPOLICY_PRIOR_PORTBIND)
extern struct net_bridge_fdb_entry *vlan_fdb_get(struct sk_buff *skb, struct net_bridge *br,
					  const unsigned char *addr);
#endif
/* Bridge group multicast address 802.1d (pg 51). */
const u8 br_group_address[ETH_ALEN] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x00 };

#if defined(TCSUPPORT_VLAN_TAG)
extern int (*remove_vtag_hook)(struct sk_buff *skb, struct net_device *dev);
#endif

#if defined(TCSUPPORT_XPON_IGMP)

#if 0
int (*xpon_bridge_incoming_hook)(struct sk_buff *skb, int clone) = NULL;
EXPORT_SYMBOL(xpon_bridge_incoming_hook);
#endif


int (*xpon_sfu_up_send_multicast_frame_hook)(struct sk_buff *skb, int clone) = NULL;

int (*xpon_sfu_up_multicast_incoming_hook)(struct sk_buff *skb, int clone) = NULL;

int (*xpon_sfu_down_multicast_incoming_hook)(struct sk_buff *skb, int clone) = NULL;

int (*xpon_sfu_up_multicast_vlan_hook)(struct sk_buff *skb, int clone) = NULL;

int (*xpon_sfu_multicast_protocol_hook)(struct sk_buff *skb) = NULL;

int (*xpon_up_igmp_uni_vlan_filter_hook)(struct sk_buff *skb) = NULL;

int (*xpon_up_igmp_ani_vlan_filter_hook)(struct sk_buff *skb) = NULL;

int (*isVlanOperationInMulticastModule_hook)(struct sk_buff *skb) = NULL;


EXPORT_SYMBOL(xpon_sfu_up_send_multicast_frame_hook);

EXPORT_SYMBOL(xpon_sfu_up_multicast_incoming_hook);

EXPORT_SYMBOL(xpon_sfu_down_multicast_incoming_hook);

EXPORT_SYMBOL(xpon_sfu_up_multicast_vlan_hook);

EXPORT_SYMBOL(xpon_sfu_multicast_protocol_hook);

EXPORT_SYMBOL(xpon_up_igmp_uni_vlan_filter_hook);

EXPORT_SYMBOL(xpon_up_igmp_ani_vlan_filter_hook);

EXPORT_SYMBOL(isVlanOperationInMulticastModule_hook);


extern int (*wan_multicast_drop_hook)(struct sk_buff *skb);
#endif

__IMEM static int br_pass_frame_up(struct sk_buff *skb)
{
	struct net_device *indev, *brdev = BR_INPUT_SKB_CB(skb)->brdev;
	struct net_bridge *br = netdev_priv(brdev);
	struct br_cpu_netstats *brstats = this_cpu_ptr(br->stats);

	u64_stats_update_begin(&brstats->syncp);
	brstats->rx_packets++;
	brstats->rx_bytes += skb->len;
	u64_stats_update_end(&brstats->syncp);

	indev = skb->dev;
	skb->dev = brdev;

	
	return NF_HOOK(NFPROTO_BRIDGE, NF_BR_LOCAL_IN, skb, indev, NULL,
		       netif_receive_skb);
}

#ifdef TCSUPPORT_BRIDGE_MAC_LIMIT
extern void br_fdb_get_mac_num(struct net_bridge *br);
extern void br_fdb_need_flush(struct net_bridge *br);
extern bool br_fdb_judge_mac_num_total(void);
extern bool br_fdb_judge_mac_num_by_port(char* devName);
extern bool br_fdb_mac_exist(struct net_bridge *br,const unsigned char *addr,char* devName);
#endif


/* note: already called with rcu_read_lock */
__IMEM int br_handle_frame_finish(struct sk_buff *skb)
{
	const unsigned char *dest = eth_hdr(skb)->h_dest;
	struct net_bridge_port *p = br_port_get_rcu(skb->dev);
	struct net_bridge *br;
	struct net_bridge_fdb_entry *dst;
#if defined (TCSUPPORT_ROUTEPOLICY_PRIOR_PORTBIND)
	struct net_bridge_fdb_entry *dst2;
#endif
	struct net_bridge_mdb_entry *mdst;
	struct sk_buff *skb2;
    int ret = 0;
	
#if defined(TCSUPPORT_XPON_IGMP)
#if 0
	typeof (xpon_bridge_incoming_hook)	xpon_bridge_incoming;
#endif
#endif
#if defined(TCSUPPORT_RA_HWNAT)
	skb->bridge_flag = 0;
#endif

	if (!p || p->state == BR_STATE_DISABLED)
		goto drop;

	/* insert into forwarding database after filtering to avoid spoofing */
	br = p->br;

	/*for dasan feature,if mac numbers of mac table > limit,drop packet*/
	#ifdef TCSUPPORT_BRIDGE_MAC_LIMIT
	br_fdb_need_flush(br);
	br_fdb_get_mac_num(br);
	if((!br_fdb_mac_exist(br,eth_hdr(skb)->h_source,skb->dev->name))&&
		(br_fdb_judge_mac_num_total()||br_fdb_judge_mac_num_by_port(skb->dev->name)))
		goto drop;
	#endif
	
	br_fdb_update(br, p, eth_hdr(skb)->h_source, skb);

#if defined(TCSUPPORT_XPON_IGMP)

#if 0
	int ret;
	xpon_bridge_incoming = rcu_dereference(xpon_bridge_incoming_hook);
	if(is_multicast_ether_addr(dest)&& xpon_bridge_incoming) 
	{
		ret = xpon_bridge_incoming(skb,1);
		if (ret>0)
		{
//			BR_INPUT_SKB_CB(skb)->brdev = br->dev;
//			br_pass_frame_up(skb);
			goto drop;	
		}
	}
#endif

    //downstream multicast operation 
    if(is_multicast_ether_addr(dest) && xpon_sfu_down_multicast_incoming_hook)
    {
        ret = xpon_sfu_down_multicast_incoming_hook(skb, 1);
        if (ret > 0 )
        {
            goto drop;  
        }
    }
    
    //send  upstream multicast to ANI, jump kernel multicast 
    if(is_multicast_ether_addr(dest) && xpon_sfu_up_send_multicast_frame_hook)
    {
        ret = xpon_sfu_up_send_multicast_frame_hook(skb, 1);
        if (ret > 0 )
        {
            goto drop;  
        }
    }
#endif
	if (is_multicast_ether_addr(dest) &&
	    br_multicast_rcv(br, p, skb))
		goto drop;

	if (p->state == BR_STATE_LEARNING)
		goto drop;

	BR_INPUT_SKB_CB(skb)->brdev = br->dev;

	/* The packet skb2 goes to the local host (NULL to skip). */
	skb2 = NULL;

	if (br->dev->flags & IFF_PROMISC)
		skb2 = skb;

	dst = NULL;


	if (is_multicast_ether_addr(dest)) {
		mdst = br_mdb_get(br, skb);
		if (mdst || BR_INPUT_SKB_CB_MROUTERS_ONLY(skb)) {
			if ((mdst && !hlist_unhashed(&mdst->mglist)) ||
			    br_multicast_is_router(br))
				skb2 = skb;
#if defined(CONFIG_PORT_BINDING) || defined(TCSUPPORT_PORTBIND)
#if !defined(TCSUPPORT_CT) 
#if defined(TCSUPPORT_FTP_THROUGHPUT)
			if (portbind_sw_hook) 
#else
#if defined(TCSUPPORT_ROUTEPOLICY_PRIOR_PORTBIND)
			if (portbind_sw_prior_hook && (portbind_sw_prior_hook(skb) == 1)) 
#else
			if (portbind_sw_hook && (portbind_sw_hook() == 1)) 
#endif
#endif
			{	
				br_multicast_pb_forward(mdst, p, skb, skb2);
			}
#endif
			else {
				br_multicast_forward(mdst, skb, skb2);
			}
#else			
			br_multicast_forward(mdst, skb, skb2);
#endif
			skb = NULL;
			if (!skb2)
				goto out;
		} else
#if defined(CONFIG_BRIDGE_IGMP_SNOOPING) && defined(TCSUPPORT_IGMPSNOOPING_ENHANCE)
		{
#endif
			skb2 = skb;
#if defined(CONFIG_BRIDGE_IGMP_SNOOPING) && defined(TCSUPPORT_IGMPSNOOPING_ENHANCE)
			/*drop the muticast packet if it is not in the muticast table*/
			if(br_multicast_should_drop(br, skb)){
#if defined(TCSUPPORT_XPON_IGMP)
				if (wan_multicast_drop_hook)  return wan_multicast_drop_hook(skb);
#endif
				br_multicast_dump_packet_info(skb, 1);
				goto up;
			}
		}	
#endif	
		br->dev->stats.multicast++;
#ifdef CONFIG_TP_IGMP_SNOOPING
		if (br_igmp_mc_forward(br, skb, dest, 1, 1))				
			skb = NULL;
#endif	

	} else
	{
#if defined(CONFIG_PORT_BINDING) || defined(TCSUPPORT_PORTBIND)
#if !defined(TCSUPPORT_CT) 
#if defined(TCSUPPORT_FTP_THROUGHPUT)
		if (portbind_sw_hook) 
#else
#if defined(TCSUPPORT_ROUTEPOLICY_PRIOR_PORTBIND)
		if (portbind_sw_prior_hook && (portbind_sw_prior_hook(skb) == 1)) 
#else
		if (portbind_sw_hook && (portbind_sw_hook() == 1)) 
#endif
#endif
		{
		//printk("In port is %s\n", p->dev->name);
			dst = __br_fdb_pb_get(br, p, dest);
		}
#endif
		else {
			dst = __br_fdb_get(br, dest);
		}
#else
		dst = __br_fdb_get(br, dest);
#endif
#if defined(TCSUPPORT_ROUTEPOLICY_PRIOR_PORTBIND)
	dst2 = dst;
	dst = vlan_fdb_get(skb, br, dest); //get dest by portbind/routepolicy/vlan
	if(dst == NULL)
		dst = dst2; //not find, use orignal br_get result;
#endif
		if (dst && dst->is_local) {
		skb2 = skb;
		/* Do not forward the packet since it's local. */
		skb = NULL;
	}
	}

	if (skb) {
#if defined(TCSUPPORT_RA_HWNAT)	
		skb->bridge_flag = 1;
#endif
		if (dst)
		{
			br_forward(dst->dst, skb, skb2);
		}
		else
		{
#if defined(CONFIG_PORT_BINDING) || defined(TCSUPPORT_PORTBIND)
#if !defined(TCSUPPORT_CT) 
		#if defined(TCSUPPORT_FTP_THROUGHPUT)
			if (portbind_sw_hook) 
		#else
#if defined(TCSUPPORT_ROUTEPOLICY_PRIOR_PORTBIND)
			if (portbind_sw_prior_hook && (portbind_sw_prior_hook(skb) == 1)) 
#else
			if (portbind_sw_hook && (portbind_sw_hook() == 1)) 
		#endif
#endif
			{	
				br_flood_pb_forward(br, p, skb, skb2);
			}
#endif
			else {
				br_flood_forward(br, skb, skb2);
			}
#else
			br_flood_forward(br, skb, skb2);
#endif
		}
	}
#if defined(CONFIG_BRIDGE_IGMP_SNOOPING) && defined(TCSUPPORT_IGMPSNOOPING_ENHANCE)
up:
#endif
	if (skb2)
		return br_pass_frame_up(skb2);

out:
	return 0;
drop:
	kfree_skb(skb);
	goto out;
}

/* note: already called with rcu_read_lock */
static int br_handle_local_finish(struct sk_buff *skb)
{
	struct net_bridge_port *p = br_port_get_rcu(skb->dev);

	if(p) {
		br_fdb_update(p->br, p, eth_hdr(skb)->h_source, skb);
	}

	return 0;	 /* process further */
}

/* Does address match the link local multicast address.
 * 01:80:c2:00:00:0X
 */
static inline int is_link_local(const unsigned char *dest)
{
	__be16 *a = (__be16 *)dest;
	static const __be16 *b = (const __be16 *)br_group_address;
	static const __be16 m = cpu_to_be16(0xfff0);

	return ((a[0] ^ b[0]) | (a[1] ^ b[1]) | ((a[2] ^ b[2]) & m)) == 0;
}

/*
 * Return NULL if skb is handled
 * note: already called with rcu_read_lock
 */
__IMEM struct sk_buff *br_handle_frame(struct sk_buff *skb)
{
	struct net_bridge_port *p;
	const unsigned char *dest = eth_hdr(skb)->h_dest;
	int (*rhook)(struct sk_buff *skb);

	#if defined(TCSUPPORT_VLAN_TAG) //Byron
	struct vlan_hdr *vhdr;
	u16 vlan_id; // vlan_id
	u16 vlan_tci; // Vlan field
	#endif
	
	if (skb->pkt_type == PACKET_LOOPBACK)
		return skb;

	if (!is_valid_ether_addr(eth_hdr(skb)->h_source))
		goto drop;
#ifdef TCSUPPORT_INIC_HOST
	if(skb->protocol == ETH_P_ROM) {
		return skb;	/* continue processing */
	}
#endif

	skb = skb_share_check(skb, GFP_ATOMIC);
	if (!skb)
		return NULL;

	p = br_port_get_rcu(skb->dev);
	if(!p) {
		goto drop;
	}

	if (unlikely(is_link_local(dest))) {
		/* Pause frames shouldn't be passed up by driver anyway */
		if (skb->protocol == htons(ETH_P_PAUSE))
			goto drop;

		/* If STP is turned off, then forward */
		if (p->br->stp_enabled == BR_NO_STP && dest[5] == 0)
			goto forward;

		if (NF_HOOK(NFPROTO_BRIDGE, NF_BR_LOCAL_IN, skb, skb->dev,
			    NULL, br_handle_local_finish))
			return NULL;	/* frame consumed by filter */
		else
			return skb;	/* continue processing */
	}

#if defined(TCSUPPORT_VLAN_TAG)
	if((skb->protocol==htons(ETH_P_8021Q)) && (skb->dev->name[0]=='n')){

		vhdr = (struct vlan_hdr *)skb->data;
		vlan_tci = ntohs(vhdr->h_vlan_TCI);
		vlan_id = vlan_tci & VLAN_VID_MASK;


		// if 8021q active & vlan_id match to do remove
		// when 8021q active only vlan_id match from WAN to LAN
		// this procedure will handle 8021q de-active. VLAN pkt can transparent from WAN to LAN, Byron
		if(skb->vlan_tags[0]==vlan_id){
			if (remove_vtag_hook) {
				if (remove_vtag_hook(skb, skb->dev) == -1) {
					/* must free skb !! */					
					kfree_skb(skb);					
					return NULL;
				}
			}
		}
	}
#endif
forward:
	switch (p->state) {
	case BR_STATE_FORWARDING:
#if 1 //defined(CONFIG_PORT_BINDING) || defined(TCSUPPORT_REDIRECT_WITH_PORTMASK)
		/*_____________________________________________
		** remark packet from different lan interfac,  
		** use the highest 4 bits.
		**
		** eth0	  	0x10000000
		** eth0.1 	0x10000000
		** eth0.2 	0x20000000
		** eth0.3 	0x30000000
		** eth0.4 	0x40000000
		** ra0	  	0x50000000
		** ra1    	0x60000000
		** ra2    	0x70000000
		** ra3    	0x80000000
		** usb0   	0x90000000
		** wds0~3  	0xA0000000
		** rai0   	0xB0000000
		** rai1   	0xC0000000
		** rai2   	0xD0000000
		** rai3   	0xE0000000
		**_________________________________________
		*/
#define		WLAN_DEV_OFFSET 		5
#ifdef TCSUPPORT_WLAN_AC
#define		WLAN_AC_DEV_OFFSET 		11
#endif
#define		USB_DEV_OFFSET		9
#define		WDS_DEV_OFFSET		10
#define		DEV_OFFSET			28
		switch (skb->dev->name[0]) {
			case 'e':
		#ifdef TCSUPPORT_TC2031
				/* device name format must be eth0 */
				skb->mark |= 0x10000000;
		#else
                //single lan port
                if(!strcmp(skb->dev->name, "eth0"))
                {
                    skb->mark |= 0x10000000;
                }

				/* device name format must be eth0.x */
				if (skb->dev->name[4] == '.')
					skb->mark |= (skb->dev->name[5] - '0') << DEV_OFFSET;
		#endif
				break;
			case 'r':
		#ifdef TCSUPPORT_WLAN_AC
				if (skb->dev->name[2] == 'i')
					/* device name must be raix */
					skb->mark |= ((skb->dev->name[3] - '0') + WLAN_AC_DEV_OFFSET) << DEV_OFFSET;
				else
		#endif
				/* device name must be rax */
				skb->mark |= ((skb->dev->name[2] - '0') + WLAN_DEV_OFFSET) << DEV_OFFSET;
				break;
			case 'u':
				/* device name must be usbx */
				skb->mark |= ((skb->dev->name[3] - '0') + USB_DEV_OFFSET) << DEV_OFFSET;
				break;
			case 'w':
				/* device name must be wdsx */
				skb->mark |= (WDS_DEV_OFFSET) << DEV_OFFSET;
				break;
			default:
				break;
		}
#endif	
		rhook = rcu_dereference(br_should_route_hook);
		if (rhook != NULL) {
			if (rhook(skb))
				return skb;
			dest = eth_hdr(skb)->h_dest;
		}
		/* fall through */
	case BR_STATE_LEARNING:
		if (!compare_ether_addr(p->br->dev->dev_addr, dest))
			skb->pkt_type = PACKET_HOST;

		NF_HOOK(NFPROTO_BRIDGE, NF_BR_PRE_ROUTING, skb, skb->dev, NULL,
			br_handle_frame_finish);
		break;
	default:
drop:
		kfree_skb(skb);
	}
	return NULL;
}
