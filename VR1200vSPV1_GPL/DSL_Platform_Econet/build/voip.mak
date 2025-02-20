export CC = $(TOOLPREFIX)gcc
export LD = $(TOOLPREFIX)ld
export AR = $(TOOLPREFIX)ar
export STRIP = $(TOOLPREFIX)strip

export APP_CMM_DIR = $(PRIVATE_APPS_PATH)/user
export APP_PJSIP_DIR = $(PUBLIC_APPS_PATH)/pjsip_1.10
export APP_VOIP_DIR = $(PRIVATE_APPS_PATH)/voip

VENDOR_CFLAGS :=

ifeq ($(strip $(INCLUDE_CPU_TC3182)),y)
sinclude $(KERNELPATH)/.config    
sinclude $(TOP_PATH)/$(SUPPLIER)/Project/profile/RT/$(CPU_TYPE)/$(CPU_TYPE).profile

MODULES_TDI_DIR := $(TOP_PATH)/$(SUPPLIER)/modules/private/voip/telephony_drv
SUPPLIER_VOIP_DIR := $(TOP_PATH)/$(SUPPLIER)/apps/private/TC_VOIP_API
DSP_DYNAMIC_LIB :=

DSP_CFLAGS := -O2 -Wall -mips32r2 -msoft-float -muclibc -DOSAL_PTHREADS
DSP_OBJ := $(SUPPLIER_VOIP_DIR)/voip_api_server/DSP_API/D2/tcDSPAPI.o

DSP_INCLUDE := -I$(SUPPLIER_VOIP_DIR)/include
DSP_INCLUDE += -I$(MODULES_TDI_DIR)

ifneq ($(strip $(INCLUDE_VOIP_WITH_DSP_D2_2S1O)),)
DSP_API_DIR := $(SUPPLIER_VOIP_DIR)/voip_api_server/DSP_API/D2/EDDY1_R_1_5_99_TDI
DSP_STATIC_LIB := $(DSP_API_DIR)/lib.m34k3/ve_vtsp.lib $(DSP_API_DIR)/lib.m34k3/osal_user.lib 
export TCSUPPORT_VOIP=y
export TCSUPPORT_VOIP_WITH_DSP_D2_2S1O=y
export TEL=2S1O
export SLIC=ZARLINK
VENDOR_CFLAGS += -DRALINK

DSP_INCLUDE += -I$(DSP_API_DIR)/include.m34k3
endif # INCLUDE_VOIP_WITH_DSP_D2_2S1O

#DSP_CFLAGS += -DVTSP_DEBUG_NETLOG

#add by huangzegeng
ifeq ($(strip $(TCSUPPORT_SDRAM_32M)),y)
VENDOR_CFLAGS += -DSUPPORT_SDRAM_32M=1
endif
#end by huangzegeng

endif  # INCLUDE_CPU_TC3182

ifeq ($(strip $(INCLUDE_CPU_AR9344)),y)
DSP_API_DIR		:= $(MODULES_PATH)/../public/phone_pjsip/pjsip
SUPPLIER_VOIP_DIR	:= $(DSP_API_DIR)/intall
DSP_STATIC_LIB		:= $(DSP_API_DIR)/pjengine/media_engine_api.o -L$(DSP_API_DIR)/install/lib -lpjatheros
DSP_OBJ			:= 
VENDOR_CFLAGS		+= -DVOIP_ATHEROS=1 -I$(MODULES_PATH)/../public/phone_pjsip/media_api
DSP_INCLUDE		+= -I$(DSP_API_DIR)/pjengine
export BOARD_TYPE=db12x
###### just for build #####
export VOIP_API_RUN=y
export PCM_DI_GPIO=2
export PCM_DO_GPIO=1
export PCM_CLK_GPIO=0
export PCM_FS_GPIO=3

export SPI_DI_GPIO=8
export SPI_DO_GPIO=7
export SPI_CLK_GPIO=6
export SLIC_SPI_CS1_GPIO=17
#export SLIC_SPI_CS2_GPIO=18

export SLIC_RESET_GPIO=13
##############################

endif # INCLUDE_CPU_AR9344

#yuchuwei
ifeq ($(strip $(INCLUDE_CPU_RT63368)),y)
sinclude $(KERNELPATH)/.config    
sinclude $(TOP_PATH)/$(SUPPLIER)/Project/profile/RT/rt63368_demo/rt63368_demo.profile

SUPPLIER_VOIP_DIR := $(TOP_PATH)/$(SUPPLIER)/apps/private/voip
DSP_DYNAMIC_TARGET := $(SUPPLIER_VOIP_DIR)/eva
DSP_STATIC_LIB := -L$(SUPPLIER_VOIP_DIR)/MTK_SIP/install/lib -lslic_user -lvdsp_user -lsyss -lbase
DSP_DYNAMIC_LIB := -L$(SUPPLIER_VOIP_DIR)/eva/bin -ladam -lgdi_mtk

DSP_CFLAGS := -O2 -Wall -mips32r2 -msoft-float -muclibc -DOSAL_PTHREADS
DSP_INCLUDE += -I$(KERNELPATH)/arch/mips/include/
DSP_INCLUDE += -I$(KERNELPATH)/include/
DSP_OBJ := $(SUPPLIER_VOIP_DIR)/mtkDSPAPI.o

DSP_INCLUDE += -I$(SUPPLIER_VOIP_DIR)/eva/common
DSP_INCLUDE += -I$(SUPPLIER_VOIP_DIR)/eva/adam
DSP_INCLUDE += -I$(SUPPLIER_VOIP_DIR)/eva/gdi_mtk
DSP_INCLUDE += -I$(MODULES_MTK_FXS3_DIR)/include -I$(MODULES_MTK_OVDSP_DIR)/include

VENDOR_CFLAGS += -DRALINK -DMTK

DSP_CFLAGS += -DVTSP_DEBUG_NETLOG -DDSPID_MTK

export PLATFORM:=tc3182 
export DSP:=mtk
export TCSUPPORT_VOIP=y
export VOIP_DSP=MTK

#add by huangzegeng
ifeq ($(strip $(TCSUPPORT_SDRAM_32M)),y)
VENDOR_CFLAGS += -DSUPPORT_SDRAM_32M=1
endif
#end by huangzegeng

ifeq ($(KERNELVERSION), 2.6.36)
MODULES_MTK_FXS3_DIR:= $(MODULES_PATH)/voip_2.6.36/DSP/MTK/mod-fxs3
MODULES_MTK_OVDSP_DIR:= $(MODULES_PATH)/voip_2.6.36/DSP/MTK/mod-ovdsp
#APP_MTKSIP_DIR:=$(MODULES_PATH)/voip_2.6.36/MTK_SIP
else
MODULES_MTK_FXS3_DIR:= $(MODULES_PATH)/voip/DSP/MTK/mod-fxs3
MODULES_MTK_OVDSP_DIR:= $(MODULES_PATH)/voip/DSP/MTK/mod-ovdsp
#APP_MTKSIP_DIR:=$(MODULES_PATH)/voip/MTK_SIP
endif
DSP_MTK_API_DIR := $(TOP_PATH)/$(SUPPLIER)/apps/private/voip
DSP_MTK_INCLUDE = -I$(MODULES_MTK_FXS3_DIR)/include -I$(MODULES_MTK_OVDSP_DIR)/include
DSP_MTK_API_LIB := -L $(MODULES_MTK_FXS3_DIR)/ -lslic_user -L $(MODULES_MTK_OVDSP_DIR)/ -lvdsp_user -L$(DSP_MTK_API_DIR)/MTK_SIP/install/lib/ -lsyss -lbase
export MODULES_MTK_FXS3_DIR MODULES_MTK_OVDSP_DIR DSP_MTK_API_DIR DSP_MTK_INCLUDE DSP_MTK_API_LIB
endif  # INCLUDE_CPU_RT63368

#yuchuwei. modify from rt63365,2016-04-28
ifeq ($(strip $(INCLUDE_CPU_MT7513)),y)
sinclude $(KERNELPATH)/.config 
#This is included in build/Makefile   
#sinclude $(TOP_PATH)/$(SUPPLIER)/Project/profile/RT/en7512_demo/en7512_demo.profile

SUPPLIER_VOIP_DIR := $(TOP_PATH)/$(SUPPLIER)/apps/private/voip
DSP_DYNAMIC_TARGET := $(SUPPLIER_VOIP_DIR)
DSP_STATIC_LIB := -L$(SUPPLIER_VOIP_DIR)/MTK_SIP/install/lib -lslic_user -lvdsp_user -lsyss -lbase
DSP_DYNAMIC_LIB := -L$(SUPPLIER_VOIP_DIR)/eva/bin -ladam -lgdi_mtk

DSP_CFLAGS := -O2 -mips32r2 -msoft-float -muclibc -DOSAL_PTHREADS -lrt -lm
DSP_INCLUDE += -I$(KERNELPATH)/arch/mips/include/
DSP_INCLUDE += -I$(KERNELPATH)/include/
DSP_OBJ := $(SUPPLIER_VOIP_DIR)/mtkDSPAPI.o

DSP_INCLUDE += -I$(SUPPLIER_VOIP_DIR)/eva/common
DSP_INCLUDE += -I$(SUPPLIER_VOIP_DIR)/eva/adam
DSP_INCLUDE += -I$(SUPPLIER_VOIP_DIR)/eva/gdi_mtk
DSP_INCLUDE += -I$(MODULES_MTK_FXS3_DIR)/include -I$(MODULES_MTK_OVDSP_DIR)/include

VENDOR_CFLAGS += -DRALINK -DMTK

DSP_CFLAGS += -DVTSP_DEBUG_NETLOG -DDSPID_MTK

export PLATFORM:=tc3182 
export DSP:=mtk
export TCSUPPORT_VOIP=y
export VOIP_DSP=MTK

#by yuchuwei.Because the value of TCSUPPORT_SDRAM_32M may be changed from y to 1.
#add by huangzegeng
ifneq ($(strip $(TCSUPPORT_SDRAM_32M)),)
VENDOR_CFLAGS += -DSUPPORT_SDRAM_32M=1
endif
#end by huangzegeng

ifeq ($(KERNELVERSION), 2.6.36)
MODULES_MTK_FXS3_DIR:= $(MODULES_PATH)/voip_2.6.36/DSP/MTK/mod-fxs3
MODULES_MTK_OVDSP_DIR:= $(MODULES_PATH)/voip_2.6.36/DSP/MTK/mod-ovdsp
else
MODULES_MTK_FXS3_DIR:= $(MODULES_PATH)/voip/DSP/MTK/mod-fxs3
MODULES_MTK_OVDSP_DIR:= $(MODULES_PATH)/voip/DSP/MTK/mod-ovdsp
endif

APP_MTKSIP_DIR:=$(SUPPLIER_VOIP_DIR)/MTK_SIP
DSP_MTK_API_DIR := $(TOP_PATH)/$(SUPPLIER)/apps/private/voip
DSP_MTK_INCLUDE = -I$(MODULES_MTK_FXS3_DIR)/include -I$(MODULES_MTK_OVDSP_DIR)/include
DSP_MTK_API_LIB := -L $(MODULES_MTK_FXS3_DIR)/ -lslic_user -L $(MODULES_MTK_OVDSP_DIR)/ -lvdsp_user -L$(DSP_MTK_API_DIR)/MTK_SIP/install/lib/ -lsyss -lbase
export MODULES_MTK_FXS3_DIR MODULES_MTK_OVDSP_DIR DSP_MTK_API_DIR DSP_MTK_INCLUDE DSP_MTK_API_LIB APP_MTKSIP_DIR
endif  # INCLUDE_CPU_MT7513




#Wang Haobin
ifeq ($(strip $(INCLUDE_CPU_VR288)),y)
sinclude $(KERNELPATH)/.config    
#sinclude $(TOP_PATH)/$(SUPPLIER)/Project/profile/RT/rt63368_demo/rt63368_demo.profile

SUPPLIER_VOIP_DIR := $(TOP_PATH)/$(SUPPLIER)/build_dir/target-mips_r2_uClibc-0.9.30.1_AC750/voip_apps
LTQ_TAPI_INCLUDE := $(TOP_PATH)/$(SUPPLIER)/build_dir/linux-ltqcpe_AC750/drv_tapi-4.10.4.2/include
LTQ_VMMC_INCLUDE := $(TOP_PATH)/$(SUPPLIER)/build_dir/linux-ltqcpe_AC750/drv_vmmc-1.15.0.4/include
LTQ_IFXOS_INCLUDE := $(TOP_PATH)/$(SUPPLIER)/build_dir/target-mips_r2_uClibc-0.9.30.1_AC750/lib_ifxos-1.5.17/src/include
TP_APPS_USER_INCLUDE := $(TOP_PATH)/apps/private/user/include
TP_APPS_USER_CMM_INCLUDE := $(TOP_PATH)/apps/private/user/clibs/cmm_lib/include

LTQ_DECT_TOOLKIT_INCLUDE := $(TOP_PATH)/$(SUPPLIER)/build_dir/target-mips_r2_uClibc-0.9.30.1_AC750/ifx_dect-3.3.5.31/dect_ifx_stack_toolkit/stack_h -I$(TOP_PATH)/$(SUPPLIER)/build_dir/target-mips_r2_uClibc-0.9.30.1_AC750/ifx_dect-3.3.5.31/dect_ifx_stack_toolkit/stack/COM/h -I$(TOP_PATH)/$(SUPPLIER)/build_dir/target-mips_r2_uClibc-0.9.30.1_AC750/ifx_dect-3.3.5.31/dect_ifx_stack_toolkit/stack/CSU/h -I$(TOP_PATH)/$(SUPPLIER)/build_dir/target-mips_r2_uClibc-0.9.30.1_AC750/ifx_dect-3.3.5.31/dect_ifx_stack_toolkit/stack/DPSU/h -I$(TOP_PATH)/$(SUPPLIER)/build_dir/target-mips_r2_uClibc-0.9.30.1_AC750/ifx_dect-3.3.5.31/dect_ifx_stack_toolkit/stack/ESU/h -I$(TOP_PATH)/$(SUPPLIER)/build_dir/target-mips_r2_uClibc-0.9.30.1_AC750/ifx_dect-3.3.5.31/dect_ifx_stack_toolkit/stack/LAU/h -I$(TOP_PATH)/$(SUPPLIER)/build_dir/target-mips_r2_uClibc-0.9.30.1_AC750/ifx_dect-3.3.5.31/dect_ifx_stack_toolkit/stack/MU/h -I$(TOP_PATH)/$(SUPPLIER)/build_dir/target-mips_r2_uClibc-0.9.30.1_AC750/ifx_dect-3.3.5.31/dect_ifx_stack_toolkit/stack/SMSU/h -I$(TOP_PATH)/$(SUPPLIER)/build_dir/target-mips_r2_uClibc-0.9.30.1_AC750/ifx_dect-3.3.5.31/dect_ifx_stack_toolkit/stack/USU/h -I$(TOP_PATH)/$(SUPPLIER)/build_dir/target-mips_r2_uClibc-0.9.30.1_AC750/ifx_dect-3.3.5.31/dect_ifx_catiq_stack

LTQ_DSP_UTILS_INCLUDE := -I$(TOP_PATH)/$(SUPPLIER)/build_dir/target-mips_r2_uClibc-0.9.30.1_AC750/ifx_voip-2.6.0.40/voip_ifx_gateway_appln/Utils/h \
-I$(TOP_PATH)/$(SUPPLIER)/build_dir/target-mips_r2_uClibc-0.9.30.1_AC750/ifx_voip-2.6.0.40/voip_ifx_gateway_appln/Interface \
-I$(TOP_PATH)/$(SUPPLIER)/build_dir/target-mips_r2_uClibc-0.9.30.1_AC750/ifx_voip-2.6.0.40/voip_ifx_common/h

LTQ_DECT_LIB := $(TOP_PATH)/$(SUPPLIER)/build_dir/target-mips_r2_uClibc-0.9.30.1_AC750/ifx_dect-3.3.5.31/lib -ldtk -ldectstack 
LTQ_VOIP_COMM_LIB := $(TOP_PATH)/$(SUPPLIER)/build_dir/target-mips_r2_uClibc-0.9.30.1_AC750/ifx_voip-2.6.0.40/voip_ifx_common/lib -lTimer -lipc -lDbg -lltqos
LTQ_IFX_OS_LIB := $(TOP_PATH)/$(SUPPLIER)/build_dir/target-mips_r2_uClibc-0.9.30.1_AC750/lib_ifxos-1.5.17/src -lifxos
TP_CUTIL_LIB := $(TOP_PATH)/apps/private/bins/lantiq_4.3.3/libs -lcutil

DSP_DYNAMIC_TARGET :=
DSP_STATIC_LIB := -L$(LTQ_DECT_LIB) -L$(LTQ_VOIP_COMM_LIB) -L$(LTQ_IFX_OS_LIB) -L$(TP_CUTIL_LIB) -L$(OS_LIB_PATH) -lcutil -los -lcmm -llte -lxml -lrt
DSP_DYNAMIC_LIB :=
DSP_CFLAGS := -O2 -Wall -mips32r2 -msoft-float -muclibc -DOSAL_PTHREADS
DSP_INCLUDE += -I$(KERNELPATH)/arch/mips/include/
DSP_INCLUDE += -I$(KERNELPATH)/include/
DSP_OBJ := $(SUPPLIER_VOIP_DIR)/ltq_dsp_common.o $(SUPPLIER_VOIP_DIR)/ltq_coder_cfg.o $(SUPPLIER_VOIP_DIR)/ltq_tapi_cfg.o $(SUPPLIER_VOIP_DIR)/ltq_fxs.o \
			$(SUPPLIER_VOIP_DIR)/ltq_dect.o $(SUPPLIER_VOIP_DIR)/ltq_dect_config.o \
			$(SUPPLIER_VOIP_DIR)/ltq_dect_stack.o $(SUPPLIER_VOIP_DIR)/ltq_timer.o $(SUPPLIER_VOIP_DIR)/ltq_dect_listAccess.o \
			$(SUPPLIER_VOIP_DIR)/pncap/etp.o $(SUPPLIER_VOIP_DIR)/pncap/pie_read.o $(SUPPLIER_VOIP_DIR)/pncap/pie_write.o $(SUPPLIER_VOIP_DIR)/pncap/utils.o 

DSP_INCLUDE += -I$(SUPPLIER_VOIP_DIR)/include
DSP_INCLUDE += -I$(SUPPLIER_VOIP_DIR)/pncap
DSP_INCLUDE += -I$(LTQ_TAPI_INCLUDE)/
DSP_INCLUDE += -I$(LTQ_VMMC_INCLUDE)/
DSP_INCLUDE += -I$(LTQ_IFXOS_INCLUDE)/
DSP_INCLUDE += -I$(TP_APPS_USER_INCLUDE)/
DSP_INCLUDE += -I$(TP_APPS_USER_CMM_INCLUDE)/
DSP_INCLUDE += -I$(LTQ_DECT_TOOLKIT_INCLUDE)/
DSP_INCLUDE += $(LTQ_DSP_UTILS_INCLUDE)/
VENDOR_CFLAGS += -DLANTIQ

DSP_CFLAGS += -DVTSP_DEBUG_NETLOG -D__LINUX_OS_FC__ -DFT -DF_DECT_STACK_LIBRARY -DDECT_SUPPORT -g -D__PIN_CODE__ -DCATIQ -DIPV4 -DLINUX -D__LINUX__ -DDEBUG_ENABLED -DDBG_LVL_DEVELOPMENT -DDEV_DEBUG -DDECT_SUPPORT -D__PSTN_ROUTE__ -DENABLE_PIN_CODE_CFG_THR_PHONE -DDEBUG_COSIC -DENABLE_PAGING -DIFX_CALLMGR -DNEW_TIMER -DCATIQ -DDECT_NG -DDECT_NG_AS -DDECT_NG_WBS -DDECT_NG_LIST_PAGE -DCATIQ_UPLANE -DLIST_SUPPORT -DFW_DOWNLOAD -DCAT_IQ2_0 -DENABLE_ENCRYPTION
export PLATFORM:= 
export DSP:=lantiq
#export TCSUPPORT_VOIP=y
export VOIP_DSP=LANTIQ

#add by huangzegeng
#ifeq ($(strip $(TCSUPPORT_SDRAM_32M)),y)
#VENDOR_CFLAGS += -DSUPPORT_SDRAM_32M=1
#endif
#end by huangzegeng

ifeq ($(KERNELVERSION), 2.6.36)
MODULES_MTK_FXS3_DIR:= #$(MODULES_PATH)/voip_2.6.36/DSP/MTK/mod-fxs3
MODULES_MTK_OVDSP_DIR:= #$(MODULES_PATH)/voip_2.6.36/DSP/MTK/mod-ovdsp
#APP_MTKSIP_DIR:=$(MODULES_PATH)/voip_2.6.36/MTK_SIP
else
MODULES_MTK_FXS3_DIR:= #$(MODULES_PATH)/voip/DSP/MTK/mod-fxs3
MODULES_MTK_OVDSP_DIR:= #$(MODULES_PATH)/voip/DSP/MTK/mod-ovdsp
#APP_MTKSIP_DIR:=$(MODULES_PATH)/voip/MTK_SIP
endif
#DSP_MTK_API_DIR := $(TOP_PATH)/$(SUPPLIER)/apps/private/voip
#DSP_MTK_INCLUDE = -I$(MODULES_MTK_FXS3_DIR)/include -I$(MODULES_MTK_OVDSP_DIR)/include
#DSP_MTK_API_LIB := -L $(MODULES_MTK_FXS3_DIR)/ -lslic_user -L $(MODULES_MTK_OVDSP_DIR)/ -lvdsp_user -L$(DSP_MTK_API_DIR)/MTK_SIP/install/lib/ -lsyss -lbase
#export MODULES_MTK_FXS3_DIR MODULES_MTK_OVDSP_DIR DSP_MTK_API_DIR DSP_MTK_INCLUDE DSP_MTK_API_LIB
endif  # INCLUDE_CPU_VR288



ifneq ($(strip $(CONFIG_NR_CPUS)), )
VENDOR_CFLAGS += -DCONFIG_NR_CPUS=$(CONFIG_NR_CPUS)
endif # CONFIG_NR_CPUS

export DSP_DYNAMIC_TARGET
export DSP_DYNAMIC_LIB
export DSP_STATIC_LIB
export DSP_OBJ
export DSP_CFLAGS
export DSP_INCLUDE

ifneq ($(strip $(INCLUDE_VOIP)),)
export INCLUDE_VOIP
export SUPPLIER

# now configuring voip locale settings

VOIP_LOCALE := -I$(APP_VOIP_DIR)/inc
ifneq ($(strip $(VOIP_LOCALE_ALL)),)
VOIP_LOCALE += -DVOIP_CFG_ALL
else
VOIP_LOCALE += $(shell cat config/$(MODEL).config | sed -n 's/=y$$//p' | sed -n 's/^VOIP_CFG/-D&/p')
endif # VOIP_LOCALE_ALL
export VOIP_LOCALE

VOIP_DFLAGS := -DINCLUDE_VOIP
VOIP_CFLAGS := $(VOIP_LOCALE)
VOIP_CFLAGS += -I$(APP_VOIP_DIR)/inc/client -I$(SUPPLIER_VOIP_DIR)/include
VOIP_CFLAGS += -I$(APP_PJSIP_DIR)/pjlib/include
VOIP_CFLAGS += -I$(OS_LIB_PATH)/include -I$(TP_MODULES_PATH)/voip

ifeq ($(strip $(CONFIG_IP_MULTIPLE_TABLES)), y)
VOIP_CFLAGS += -DCONFIG_IP_MULTIPLE_TABLES
endif

ifeq ($(strip $(INCLUDE_DSP_SOCKET_OPEN)), y)
VOIP_CFLAGS += -DINCLUDE_DSP_SOCKET_OPEN
export INCLUDE_DSP_SOCKET_OPEN
endif

export NUM_FXS_CHANNELS := $(NUM_FXS_CHANNELS)
VOIP_DFLAGS += -DINCLUDE_FXS_NUM=$(NUM_FXS_CHANNELS)
VOIP_CFLAGS += -DNUM_FXS_CHANNELS=$(NUM_FXS_CHANNELS)
ifeq ($(strip $(INCLUDE_CPU_AR9344)),y)
export CHANNEL = $(NUM_FXS_CHANNELS)
endif

ifneq ($(strip $(INCLUDE_DMZ)),)
VOIP_CFLAGS += -DINCLUDE_DMZ
endif

ifneq ($(strip $(INCLUDE_DIGITMAP)),)
export INCLUDE_DIGITMAP=y
VOIP_DFLAGS += -DINCLUDE_DIGITMAP
endif

ifneq ($(strip $(INCLUDE_USB_VOICEMAIL)),)
export INCLUDE_USB_VOICEMAIL=y
VOIP_DFLAGS += -DINCLUDE_USB_VOICEMAIL

ifneq ($(strip $(INCLUDE_USBVM_MODULE)),)
#ifeq ($(strip $(CONFIG_HZ)), $(shell echo $$[$(CONFIG_HZ) / 100 * 100]))
export INCLUDE_USBVM_MODULE=y
VOIP_CFLAGS += -DINCLUDE_USBVM_MODULE
#endif
endif

# configuration about broadsoft certification
ifneq ($(strip $(INCLUDE_BROADSOFT)),)
export INCLUDE_BROADSOFT=y
VOIP_DFLAGS += -DINCLUDE_BROADSOFT
endif

endif

ifneq ($(strip $(INCLUDE_CALLLOG)),)
export INCLUDE_CALLLOG=y
VOIP_DFLAGS += -DINCLUDE_CALLLOG
endif

ifneq ($(strip $(INCLUDE_PSTN)),)

export INCLUDE_PSTN = y
VOIP_DFLAGS += -DINCLUDE_PSTN
ifneq ($(strip $(INCLUDE_PSTN_LIFELINE)),)
export INCLUDE_PSTN_LIFELINE=y
VOIP_DFLAGS += -DINCLUDE_PSTN_LIFELINE
endif
ifneq ($(strip $(INCLUDE_PSTN_POLREV)),)
export INCLUDE_PSTN_POLREV=y
VOIP_DFLAGS += -DINCLUDE_PSTN_POLREV
endif
ifneq ($(strip $(INCLUDE_PSTN_GATEWAY)),)
export INCLUDE_PSTN_GATEWAY=y
VOIP_DFLAGS += -DINCLUDE_PSTN_GATEWAY
endif

endif  # INCLUDE_PSTN

ifneq ($(strip $(INCLUDE_DECT)),)
export INCLUDE_DECT=y
VOIP_DFLAGS += -DINCLUDE_DECT
export NUM_DECT_CHANNELS := $(NUM_DECT_CHANNELS)
VOIP_DFLAGS += -DINCLUDE_DECT_NUM=$(NUM_DECT_CHANNELS)
VOIP_CFLAGS += -DNUM_DECT_CHANNELS=$(NUM_DECT_CHANNELS)
endif

ifneq ($(strip $(INCLUDE_VOICEAPP)),)
export INCLUDE_VOICEAPP=y
VOIP_DFLAGS += -DINCLUDE_VOICEAPP
export NUM_VOICEAPP_CHANNELS := $(NUM_VOICEAPP_CHANNELS)
VOIP_DFLAGS += -DINCLUDE_VOICEAPP_NUM=$(NUM_VOICEAPP_CHANNELS)
VOIP_CFLAGS += -DNUM_VOICEAPP_CHANNELS=$(NUM_VOICEAPP_CHANNELS)
endif

export VOIP_CFLAGS += $(VENDOR_CFLAGS)
export VOIP_DFLAGS
ifneq ($(strip $(INCLUDE_CALLTHROUGH)),)
export INCLUDE_CALLTHROUGH=y
VOIP_DFLAGS += -DINCLUDE_CALLTHROUGH
endif
ifneq ($(strip $(INCLUDE_CALLFWD_THROUGH_DUT)),)
export INCLUDE_CALLFWD_THROUGH_DUT=y
VOIP_DFLAGS += -DINCLUDE_CALLFWD_THROUGH_DUT
endif

ifneq ($(strip $(INCLUDE_SIP_DOMAIN)),)
export INCLUDE_SIP_DOMAIN=y
VOIP_DFLAGS += -DINCLUDE_SIP_DOMAIN
endif

ifneq ($(strip $(INCLUDE_QDMALAN_IRQ_DYN_BINDING)),)
export INCLUDE_QDMALAN_IRQ_DYN_BINDING=y
VOIP_DFLAGS += -DINCLUDE_QDMALAN_IRQ_DYN_BINDING
endif
endif  # INCLUDE_VOIP

