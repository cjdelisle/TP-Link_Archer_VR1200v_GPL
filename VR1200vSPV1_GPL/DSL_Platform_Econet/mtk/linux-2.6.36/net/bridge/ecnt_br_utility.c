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
#include "br_private.h"
#include <linux/ip.h>
#ifdef TCSUPPORT_IPV6
#include <linux/ipv6.h>
#endif
#include <linux/udp.h>



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


/************************************************************************
*                  F U N C T I O N   D E F I N I T I O N S
*************************************************************************
*/

/****************************************************************************
**function name
	 get_client_mac_from_dhcp_packet
**description:
	get clinet mac from broadcast dhcp packet
**return 
	0: success
	-1: fail
**parameter:
	skb: the packet information
	cliMacAddr: client MAC Address
****************************************************************************/
int get_client_mac_from_dhcp_packet(struct sk_buff *skb, unsigned char *cliMacAddr)
{
	int eth_type = 0;
	int dhcp_mode = 0;
	struct iphdr *ip_hdr = NULL;
	struct udphdr *udp_hdr = NULL;
	unsigned char *p = skb->data;
	
	if(skb == NULL)
		return -1;
	if(skb->dev == NULL)
		return -1;

	p = p + 12;//point to ethernet type
	eth_type = *(unsigned short *)p;
	if(eth_type == 0x8100){
		p = p + 4;
		eth_type = *(unsigned short *)p;
		if(eth_type == 0x8100){
			p = p + 4;
			eth_type = *(unsigned short *)p;
		}
	}

	if (eth_type != ETH_P_IP){
		return -1;
	}
	
	p = p + 2;//point to ip header
	ip_hdr = (struct iphdr *)p;
	if(ip_hdr->protocol != 17) {//not udp
		return -1;
	}
	
	p = p + 20; //point to UDP header
	udp_hdr = (struct udphdr *)p;
	if(udp_hdr->source == 68 || udp_hdr->dest == 67)
	{
		dhcp_mode = 1; //dhcp boot request
	}
	else if(udp_hdr->source == 67 || udp_hdr->dest == 68)
	{
		dhcp_mode = 2; //dhcp boot reply
	}
	
	if(dhcp_mode == 0){//not dhcp
		return -1;
	}
	
	p = p + 8; //point to DHCP 
	p += 28;//poinrt to Client Mac address
	memcpy(cliMacAddr, p, 6);
	
	return 0;
	
}

/****************************************************************************
**function name
	 get_fdb_by_skb
**description:
	get net bridge fdb entry by skb information
**return 
	dst: if find match entry
	NULL: if not find match entry
**parameter:
	skb: the packet information
****************************************************************************/
struct net_bridge_fdb_entry *get_fdb_by_skb(struct sk_buff *skb)
{
	unsigned char dest[6] = {0};
	struct net_bridge_fdb_entry *dst = NULL;	
	struct net_bridge_port *p = br_port_get_rcu(skb->dev);
	struct net_bridge *br;

	if (!p || p->state == BR_STATE_DISABLED)
		return NULL;

	memcpy(dest, skb->data, sizeof(dest));
	if(is_broadcast_ether_addr(dest))
	{
		if(get_client_mac_from_dhcp_packet(skb, dest) == -1)
		{
			return dst;
		}

	}

	br = p->br;
	dst = __br_fdb_get(br, dest);

	return dst;
}
EXPORT_SYMBOL(get_fdb_by_skb);
