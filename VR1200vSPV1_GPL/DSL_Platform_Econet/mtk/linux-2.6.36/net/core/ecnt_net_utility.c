/***************************************************************
Copyright Statement:

This software/firmware and related documentation (¡°EcoNet Software¡±) 
are protected under relevant copyright laws. The information contained herein 
is confidential and proprietary to EcoNet (HK) Limited (¡°EcoNet¡±) and/or 
its licensors. Without the prior written permission of EcoNet and/or its licensors, 
any reproduction, modification, use or disclosure of EcoNet Software, and 
information contained herein, in whole or in part, shall be strictly prohibited.

EcoNet (HK) Limited  EcoNet. ALL RIGHTS RESERVED.

BY OPENING OR USING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY 
ACKNOWLEDGES AND AGREES THAT THE SOFTWARE/FIRMWARE AND ITS 
DOCUMENTATIONS (¡°ECONET SOFTWARE¡±) RECEIVED FROM ECONET 
AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON AN ¡°AS IS¡± 
BASIS ONLY. ECONET EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES, 
WHETHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED 
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, 
OR NON-INFRINGEMENT. NOR DOES ECONET PROVIDE ANY WARRANTY 
WHATSOEVER WITH RESPECT TO THE SOFTWARE OF ANY THIRD PARTIES WHICH 
MAY BE USED BY, INCORPORATED IN, OR SUPPLIED WITH THE ECONET SOFTWARE. 
RECEIVER AGREES TO LOOK ONLY TO SUCH THIRD PARTIES FOR ANY AND ALL 
WARRANTY CLAIMS RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES 
THAT IT IS RECEIVER¡¯S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD 
PARTY ALL PROPER LICENSES CONTAINED IN ECONET SOFTWARE.

ECONET SHALL NOT BE RESPONSIBLE FOR ANY ECONET SOFTWARE RELEASES 
MADE TO RECEIVER¡¯S SPECIFICATION OR CONFORMING TO A PARTICULAR 
STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND 
ECONET'S ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE ECONET 
SOFTWARE RELEASED HEREUNDER SHALL BE, AT ECONET'S SOLE OPTION, TO 
REVISE OR REPLACE THE ECONET SOFTWARE AT ISSUE OR REFUND ANY SOFTWARE 
LICENSE FEES OR SERVICE CHARGES PAID BY RECEIVER TO ECONET FOR SUCH 
ECONET SOFTWARE.
***************************************************************/

/************************************************************************
*                  I N C L U D E S
*************************************************************************
*/
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/if_vlan.h>
#include <linux/if_pppox.h>
#include <linux/ip.h>
#ifdef TCSUPPORT_IPV6
#include <linux/ipv6.h>
#endif


/************************************************************************
*                  D E F I N E S   &   C O N S T A N T S
*************************************************************************
*/

/************************************************************************
*                  M A C R O S
*************************************************************************
*/

/************************************************************************
*                  D A T A   T Y P E S
*************************************************************************
*/

/************************************************************************
*                  E X T E R N A L   D A T A   D E C L A R A T I O N S
*************************************************************************
*/

/************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
*************************************************************************
*/


/************************************************************************
*                  P U B L I C   D A T A
*************************************************************************
*/
int (*match_multicast_vtag_check)
(struct sk_buff *skb, struct net_device *vdev);
EXPORT_SYMBOL(match_multicast_vtag_check);
#if !defined(TCSUPPORT_CT_VLAN_TAG)
int (*match_multicast_vtag)(struct sk_buff *skb, struct net_device *vdev);
EXPORT_SYMBOL(match_multicast_vtag);
#endif

int (*wifi_eth_fast_tx_hook)(struct sk_buff *skb) = NULL;
EXPORT_SYMBOL(wifi_eth_fast_tx_hook);

int (*wlan_ratelimit_enqueue_hook) (struct sk_buff * skb,unsigned char direction) = NULL;
EXPORT_SYMBOL(wlan_ratelimit_enqueue_hook);

/************************************************************************
*                  F U N C T I O N   D E F I N I T I O N S
*************************************************************************
*/

/****************************************************************************
**function name
	 __vlan_proto
**description:
	get protocol via skb
**return 
	eth_type
**parameter:
	skb: the packet information
****************************************************************************/
static inline __be16 __vlan_proto(const struct sk_buff *skb)
{
	return vlan_eth_hdr(skb)->h_vlan_encapsulated_proto;
}

/****************************************************************************
**function name
	 check_ppp_udp_multicast
**description:
	check multicast packet in downstream
**return 
	0:	check ok or ignore
	-1:	fail
**parameter:
	skb: the packet information
	vdev: virtual net device
****************************************************************************/
int check_ppp_udp_multicast
(struct sk_buff *skb, struct net_device *vdev)
{

	return 0; /* check ok or ignore. */
}
EXPORT_SYMBOL(check_ppp_udp_multicast);

/****************************************************************************
**function name
	 __is_ip_udp
**description:
	check whether packet is IP udp packets.
**return 
	0:	check ok or ignore
	-1:	fail
**parameter:
	skb: the packet information
	vdev: virtual net device
****************************************************************************/
int __is_ip_udp(struct sk_buff *skb)
{
#if !defined(TCSUPPORT_CT_PON_SC) 
	return 0;
#endif
}
EXPORT_SYMBOL(__is_ip_udp);

