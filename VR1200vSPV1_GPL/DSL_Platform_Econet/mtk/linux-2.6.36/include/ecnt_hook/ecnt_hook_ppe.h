/***************************************************************
Copyright Statement:

This software/firmware and related documentation (EcoNet Software) 
are protected under relevant copyright laws. The information contained herein 
is confidential and proprietary to EcoNet (HK) Limited (EcoNet) and/or 
its licensors. Without the prior written permission of EcoNet and/or its licensors, 
any reproduction, modification, use or disclosure of EcoNet Software, and 
information contained herein, in whole or in part, shall be strictly prohibited.

EcoNet (HK) Limited  EcoNet. ALL RIGHTS RESERVED.

BY OPENING OR USING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY 
ACKNOWLEDGES AND AGREES THAT THE SOFTWARE/FIRMWARE AND ITS 
DOCUMENTATIONS (ECONET SOFTWARE) RECEIVED FROM ECONET 
AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON AN AS IS 
BASIS ONLY. ECONET EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES, 
WHETHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED 
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, 
OR NON-INFRINGEMENT. NOR DOES ECONET PROVIDE ANY WARRANTY 
WHATSOEVER WITH RESPECT TO THE SOFTWARE OF ANY THIRD PARTIES WHICH 
MAY BE USED BY, INCORPORATED IN, OR SUPPLIED WITH THE ECONET SOFTWARE. 
RECEIVER AGREES TO LOOK ONLY TO SUCH THIRD PARTIES FOR ANY AND ALL 
WARRANTY CLAIMS RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES 
THAT IT IS RECEIVERS SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD 
PARTY ALL PROPER LICENSES CONTAINED IN ECONET SOFTWARE.

ECONET SHALL NOT BE RESPONSIBLE FOR ANY ECONET SOFTWARE RELEASES 
MADE TO RECEIVERS SPECIFICATION OR CONFORMING TO A PARTICULAR 
STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND 
ECONET'S ENTIRE AND CUMULATIVE LIABILITY WITH RESPECT TO THE ECONET 
SOFTWARE RELEASED HEREUNDER SHALL BE, AT ECONET'S SOLE OPTION, TO 
REVISE OR REPLACE THE ECONET SOFTWARE AT ISSUE OR REFUND ANY SOFTWARE 
LICENSE FEES OR SERVICE CHARGES PAID BY RECEIVER TO ECONET FOR SUCH 
ECONET SOFTWARE.
***************************************************************/
#ifndef _ECNT_HOOK_PPE_H_
#define _ECNT_HOOK_PPE_H_


/************************************************************************
*                  I N C L U D E S
*************************************************************************
*/
#include "ecnt_hook.h"

/************************************************************************
*                  D E F I N E S   &   C O N S T A N T S
*************************************************************************
*/

/************************************************************************
*                  M A C R O S
*************************************************************************
*/

/************************************************************************
*                  D A T A	 T Y P E S
*************************************************************************
*/
enum {
    ECNT_DRIVER_PPE_API = 0,
};

typedef enum {
	PPE_API_ID_CLEAN_FOE_TABLE = 0,
    PPE_API_ID_DUMP_FOE_PKT,
    PPE_API_ID_ADD_FOE_ENTRY,
	PPE_API_ID_MAX_NUM
} PPE_HookFunctionID_t ;

struct ecnt_ppe_data {
	PPE_HookFunctionID_t function_id;
    int retValue;

    union {
        void *skb;
        void *hwnat_tuple;
    };
};

/************************************************************************
*                  D A T A   D E C L A R A T I O N S
*************************************************************************
*/

/************************************************************************
*                  F U N C T I O N    D E C L A R A T I O N S
#                  I N L I N E    F U N C T I O N   D E F I N I T I O N S
*************************************************************************
*/
static inline int PPE_API_CLEAN_FOE_TABLE(void) 
{
	struct ecnt_ppe_data in_data; 
	int ret = 0;
	
	in_data.function_id = PPE_API_ID_CLEAN_FOE_TABLE; 
    
	ret = __ECNT_HOOK(ECNT_PPE, ECNT_DRIVER_PPE_API, (struct ecnt_data *)&in_data);
    
	if(ret != ECNT_HOOK_ERROR)
		return in_data.retValue;
	else
		return ECNT_HOOK_ERROR;
}

static inline int PPE_API_DUMP_FOE_PKT(void *skb) 
{
	struct ecnt_ppe_data in_data; 
	int ret = 0;
	
	in_data.function_id = PPE_API_ID_DUMP_FOE_PKT;

    in_data.skb = skb;
    
	ret = __ECNT_HOOK(ECNT_PPE, ECNT_DRIVER_PPE_API, (struct ecnt_data *)&in_data);
    
	if(ret != ECNT_HOOK_ERROR)
		return in_data.retValue;
	else
		return ECNT_HOOK_ERROR;
}

static inline int PPE_API_ADD_FOE_ENTRY(void *hwnat_tuple) 
{
	struct ecnt_ppe_data in_data; 
	int ret = 0;
	
	in_data.function_id = PPE_API_ID_ADD_FOE_ENTRY;

    in_data.hwnat_tuple = hwnat_tuple;
    
	ret = __ECNT_HOOK(ECNT_PPE, ECNT_DRIVER_PPE_API, (struct ecnt_data *)&in_data);
    
	if(ret != ECNT_HOOK_ERROR)
		return in_data.retValue;
	else
		return ECNT_HOOK_ERROR;
}


#endif /* _ECNT_HOOK_PPE_H_ */


