#if 0
#if !defined(TCSUPPORT_ORN_EBTABLES)

#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <net/checksum.h>
#include <linux/if_vlan.h>
#include <linux/netfilter/x_tables.h>
#include <linux/netfilter_bridge/ebtables.h>
#include <linux/netfilter_bridge/ebt_ftos_t.h>
#ifdef CONFIG_QOS
#include <linux/qos_type.h>
#endif
//#define QOS_WANIF_MARK	0xff000

#if 1
static inline __be16 vlan_proto(const struct sk_buff *skb)
{
	return vlan_eth_hdr(skb)->h_vlan_encapsulated_proto;
}
static inline __be16 pppoe_proto(const struct sk_buff *skb)
{
	return *((__be16 *)(skb_mac_header(skb) + ETH_HLEN +
			    sizeof(struct pppoe_hdr)));
}
#endif

/*
(struct sk_buff **pskb, unsigned int hooknr,
   const struct net_device *in, const struct net_device *out,
   const void *data, unsigned int datalen)
 */

static int ebt_ftos_mt(const struct sk_buff *skb, struct xt_action_param *par)
{
	const struct ebt_ftos_info *ftosinfo = par->matchinfo;
	struct iphdr *ih = NULL;
	struct iphdr _iph;
	__u8 oldtos = 0;
#ifdef CONFIG_QOS
	__u8 tos = 0;
	int rule_no = 0;
#endif
	__u8 newtos = 0;

#if 1  /*Rodney_20090724*/
	if((skb)->protocol == htons(ETH_P_IP))
		ih = (struct iphdr *)skb_header_pointer(skb, 0, sizeof(_iph), &_iph);
	else if(((skb)->protocol == htons(ETH_P_8021Q)) && (vlan_proto(skb) == htons(ETH_P_IP)))
		ih = (struct iphdr *)(skb_mac_header(skb) + VLAN_ETH_HLEN);
    else if(((skb)->protocol == htons(ETH_P_PPP_SES)) && (pppoe_proto(skb) == htons(0x0021)))
		ih = (struct iphdr *)(skb_mac_header(skb) + ETH_HLEN +PPPOE_SES_HLEN);
	else
		ih = (struct iphdr *)skb_header_pointer(skb, 0, sizeof(_iph), &_iph);
#else
	ih = (struct iphdr *)skb_header_pointer(*pskb, 0, sizeof(_iph), &_iph);
#endif

	
	if (!skb_make_writable(skb, sizeof(struct iphdr)))
		return NF_DROP;

    oldtos = ih->tos;

#if 0
	if ( (*pskb)->mark & QOS_WANIF_MARK ) {
		tos = (ih->tos & ~ftosinfo->mask) | (ftosinfo->ftos & ftosinfo->mask);
		(*pskb)->mark |= (tos << 18);
		return ftosinfo->target | ~EBT_VERDICT_BITS;
	}
#endif
#ifdef CONFIG_QOS
	rule_no = (skb->mark & QOS_RULE_INDEX_MARK) >> 12;
	if (0 == qostype_chk(EBT_CHK_TYPE, rule_no, NULL, 0)) {
		tos = (ih->tos & ~ftosinfo->mask) | (ftosinfo->ftos & ftosinfo->mask);
		set_tos(rule_no, tos);
		return ftosinfo->target | ~EBT_VERDICT_BITS;
	}
	else {
		unset_tos(rule_no);
	}
#endif

/* Edit by ZJ, 2013-11-14 */
/* In usrspace, ftos means ftos_set, mask means ftos... */
#if 0
    ih->tos = (ih->tos & ~ftosinfo->mask) | (ftosinfo->ftos & ftosinfo->mask);
    csum_replace2(&ih->check, htons(oldtos), htons(ih->tos));
#else
	if ((ftosinfo->ftos & FTOS_SETFTOS) && (ih->tos != ftosinfo->mask)) 
	{
		ih->tos = ftosinfo->mask;
		csum_replace2(&ih->check, htons(oldtos), htons(ih->tos));
	}
	/* Such case not used and not test, just copy from 2.6.31 */
	else if (ftosinfo->ftos & FTOS_WMMFTOS)
	{
		ih->tos |= ((skb->mark >> PRIO_LOC_NFMARK) & PRIO_LOC_NFMASK) << DSCP_MASK_SHIFT;
		csum_replace2(&ih->check, htons(oldtos), htons(ih->tos));
	}
	/* Such case used for 802.1P, not have now, not test and just copy from 2.6.31 */
	else if ((ftosinfo->ftos & FTOS_8021QFTOS) && skb->protocol == __constant_htons(ETH_P_8021Q)) 
	{
		struct vlan_hdr *frame;	      
		unsigned char prio = 0;      
		unsigned short TCI;

		frame = (struct vlan_hdr *)(skb->network_header);
		TCI = ntohs(frame->h_vlan_TCI);
		prio = (unsigned char)((TCI >> 13) & 0x7);

		ih->tos |= (prio & 0xf) << DSCP_MASK_SHIFT;
		csum_replace2(&ih->check, htons(oldtos), htons(ih->tos));
	}
#endif
/* End add */

    return ftosinfo->target | ~EBT_VERDICT_BITS;
}
/*
static int ebt_target_ftos_check(const char *tablename, unsigned int hookmask,
   const struct ebt_entry *e, void *data, unsigned int datalen)
*/

static int ebt_ftos_mt_check(const struct xt_mtchk_param *par)
{
	struct ebt_ftos_info *info = (struct ebt_ftos_info *)par->matchinfo;
/*
	if (datalen != sizeof(struct ebt_ftos_info))
		return -EINVAL;
*/
	if (BASE_CHAIN && info->target == EBT_RETURN)
		return -EINVAL;
//	CLEAR_BASE_CHAIN_BIT;
	if (INVALID_TARGET)
		return -EINVAL;
	return 0;
}

static struct xt_target ftos_target __read_mostly=
{
	.name		= "ftos",
	.revision	= 0,
	.family		= NFPROTO_BRIDGE,
	.target		= ebt_ftos_mt,
	.checkentry	= ebt_ftos_mt_check,
	.targetsize	= sizeof(struct ebt_ftos_info),
	.me		= THIS_MODULE,
};

static int __init ebt_ftos_init(void)
{
	return xt_register_target(&ftos_target);
}

static void __exit ebt_ftos_fini(void)
{
	xt_unregister_target(&ftos_target);
}

module_init(ebt_ftos_init);
module_exit(ebt_ftos_fini);
MODULE_LICENSE("GPL");
#endif

#else

/*
 *  ebt_ftos
 *
 *	Authors:
 *	 Song Wang <songw@broadcom.com>
 *
 *  Feb, 2004
 *
 */

// The ftos target can be used in any chain
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <net/checksum.h>
#include <linux/if_vlan.h>
#include <linux/netfilter/x_tables.h>
#include <linux/netfilter_bridge/ebtables.h>
#include <linux/netfilter_bridge/ebt_ftos_t.h>

#include <linux/ipv6.h>
#include <net/dsfield.h>

#include <linux/netfilter/xt_DSCP.h>
//#include <linux/netfilter_ipv4/ipt_TOS.h>

/* zl moved from brcm ebt_ftos.c , 2012-7-30 */
#define PPPOE_HLEN   6
#define PPP_TYPE_IPV4   0x0021  /* IPv4 in PPP */
#define PPP_TYPE_IPV6   0x0057  /* IPv6 in PPP */
/* end--moved */

static unsigned int
ebt_target_ftos(struct sk_buff *skb, const struct xt_action_param *par)
{
	const struct ebt_ftos_t_info *ftosinfo = par->targinfo;

	struct iphdr *iph = NULL;
        struct vlan_hdr *frame;	
	unsigned char prio = 0;
	unsigned short TCI;
        /* Need to recalculate IP header checksum after altering TOS byte */
	u_int16_t diffs[2];
    struct ipv6hdr *ipv6h = NULL;

/* if VLAN frame, we need to point to correct network header */
   if (skb->protocol == __constant_htons(ETH_P_IP))
      iph = (struct iphdr *)(skb->network_header);
   else if ((skb)->protocol == __constant_htons(ETH_P_IPV6))
      ipv6h = (struct ipv6hdr *)(skb->network_header);
   else if (skb->protocol == __constant_htons(ETH_P_8021Q)) {
      if (*(unsigned short *)(skb->network_header + VLAN_HLEN - 2) == __constant_htons(ETH_P_IP))
         iph = (struct iphdr *)(skb->network_header + VLAN_HLEN);
      else if (*(unsigned short *)(skb->network_header + VLAN_HLEN - 2) == __constant_htons(ETH_P_IPV6))
         ipv6h = (struct ipv6hdr *)(skb->network_header + VLAN_HLEN);
   }
   else if (skb->protocol == __constant_htons(ETH_P_PPP_SES)) {
      if (*(unsigned short *)(skb->network_header + PPPOE_HLEN) == PPP_TYPE_IPV4)
         iph = (struct iphdr *)(skb->network_header + PPPOE_HLEN + 2);
      else if (*(unsigned short *)(skb->network_header + PPPOE_HLEN) == PPP_TYPE_IPV6)
         ipv6h = (struct ipv6hdr *)(skb->network_header + PPPOE_HLEN + 2);
   }
   /* if not IP header, do nothing. */
   if ((iph == NULL) && (ipv6h == NULL))
	   return ftosinfo->target;

   if ( iph != NULL ) //IPv4
   {
	if ((ftosinfo->ftos_set & FTOS_SETFTOS) && (iph->tos != ftosinfo->ftos)) {
                //printk("ebt_target_ftos:FTOS_SETFTOS .....\n");
		diffs[0] = htons(iph->tos) ^ 0xFFFF;
		iph->tos = ftosinfo->ftos;
		diffs[1] = htons(iph->tos);

		iph->check = csum_fold(csum_partial((char *)diffs,
		                                    sizeof(diffs),
		                                    iph->check^0xFFFF));	
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// member below is removed
//		(*pskb)->nfcache |= NFC_ALTERED;
	} else if (ftosinfo->ftos_set & FTOS_WMMFTOS) {
#if 0
	    //printk("ebt_target_ftos:FTOS_WMMFTOS .....0x%08x\n", (*pskb)->mark & 0xf);
      diffs[0] = htons(iph->tos) ^ 0xFFFF;
      iph->tos |= ((skb->mark >> PRIO_LOC_NFMARK) & PRIO_LOC_NFMASK) << DSCP_MASK_SHIFT;
      diffs[1] = htons(iph->tos);
      iph->check = csum_fold(csum_partial((char *)diffs,
		                                    sizeof(diffs),
		                                    iph->check^0xFFFF));
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// member below is removed
//        (*pskb)->nfcache |= NFC_ALTERED;
#endif
	} else if ((ftosinfo->ftos_set & FTOS_8021QFTOS) && skb->protocol == __constant_htons(ETH_P_8021Q)) {
#if 0
      struct vlan_hdr *frame;	
      unsigned char prio = 0;
      unsigned short TCI;

      frame = (struct vlan_hdr *)(skb->network_header);
      TCI = ntohs(frame->h_vlan_TCI);
      prio = (unsigned char)((TCI >> 13) & 0x7);
        //printk("ebt_target_ftos:FTOS_8021QFTOS ..... 0x%08x\n", prio);
      diffs[0] = htons(iph->tos) ^ 0xFFFF;
      iph->tos |= (prio & 0xf) << DSCP_MASK_SHIFT;
      diffs[1] = htons(iph->tos);
      iph->check = csum_fold(csum_partial((char *)diffs,
		                                    sizeof(diffs),
		                                    iph->check^0xFFFF)); 
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// member below is removed
//        (*pskb)->nfcache |= NFC_ALTERED;
#endif
	}
   }
   else //IPv6
   {
      __u8 tos;

      /* TOS consists of priority field and first 4 bits of flow_lbl */
      tos = ipv6_get_dsfield((struct ipv6hdr *)(skb->network_header));

      if ((ftosinfo->ftos_set & FTOS_SETFTOS) && (tos != ftosinfo->ftos))
      {
         //printk("ebt_target_ftos:FTOS_SETFTOS .....\n");
         ipv6_change_dsfield((struct ipv6hdr *)(skb->network_header), 0, ftosinfo->ftos);
      } 
      else if (ftosinfo->ftos_set & FTOS_WMMFTOS) 
      {
#if 0
         //printk("ebt_target_ftos:FTOS_WMMFTOS .....0x%08x\n", 
	     tos |= ((skb->mark >> PRIO_LOC_NFMARK) & PRIO_LOC_NFMASK) << DSCP_MASK_SHIFT;
         ipv6_change_dsfield((struct ipv6hdr *)(skb->network_header), 0, tos);
#endif
      } 
      else if ((ftosinfo->ftos_set & FTOS_8021QFTOS) && 
               skb->protocol == __constant_htons(ETH_P_8021Q)) 
      {
#if 0
         struct vlan_hdr *frame;	
         unsigned char prio = 0;
         unsigned short TCI;

         frame = (struct vlan_hdr *)(skb->network_header);
         TCI = ntohs(frame->h_vlan_TCI);
         prio = (unsigned char)((TCI >> 13) & 0x7);
         //printk("ebt_target_ftos:FTOS_8021QFTOS ..... 0x%08x\n", prio);
         tos |= (prio & 0xf) << DSCP_MASK_SHIFT;
         ipv6_change_dsfield((struct ipv6hdr *)(skb->network_header), 0, tos);
#endif
      }
   }
	return ftosinfo->target;
}

static int ebt_target_ftos_check(const struct xt_tgchk_param *par)
{
	const struct ebt_ftos_t_info *info = par->targinfo;

	if (BASE_CHAIN && info->target == EBT_RETURN)
		return -EINVAL;
	//CLEAR_BASE_CHAIN_BIT;
	if (INVALID_TARGET)
		return -EINVAL;
	return 0;
}

static struct xt_target ftos_target __read_mostly =
{
        .name           = EBT_FTOS_TARGET,
	.revision	= 0,
	.family		= NFPROTO_BRIDGE,
        .target         = ebt_target_ftos,
        .checkentry     = ebt_target_ftos_check,
	.targetsize	= sizeof(struct ebt_ftos_t_info),
        .me             = THIS_MODULE,
};

static int __init init(void)
{
	return xt_register_target(&ftos_target);
}

static void __exit fini(void)
{
	xt_unregister_target(&ftos_target);
}

module_init(init);
module_exit(fini);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Song Wang, songw@broadcom.com");
MODULE_DESCRIPTION("Target to overwrite the full TOS byte in IP header");

#endif
