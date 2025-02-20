#**************************************************************************** 
# 
#  Copyright(c) 2010-2011 Shenzhen TP-Link Corporation Limited 
#  All Rights Reserved 
#  YangXv <yangxu@tp-link.com.cn> 
# 
#***************************************************************************

# Top-level Makefile for all linux platforms 
TP_CFLAGS = 

ifndef MODEL
$(error "You have to define MODEL, such as VG3631V1")
endif

include config/$(MODEL).config

ifeq ($(strip $(INCLUDE_CPU_TC3182)),y)
CPU_TYPE=tc3182
endif

ifeq ($(strip $(INCLUDE_CPU_TC3162U)),y)
CPU_TYPE=tc3162u
endif

#zl added 2011-10-18
ifeq ($(strip $(INCLUDE_CPU_RTL8672)),y)
CPU_TYPE=rtl8672
endif

ifeq ($(strip $(INCLUDE_CPU_VR268)),y)
CPU_TYPE=vr268
include config/config.wifi
endif

# wzy added 2013-7-17
ifeq ($(strip $(INCLUDE_CPU_VR288)),y)
CPU_TYPE=vr288
# include config/config.wifi
endif
#end--added


# yangxv added 2013-12
ifeq ($(strip $(INCLUDE_CPU_AR368)),y)
CPU_TYPE=ar368
endif
#end--ar368
ifeq ($(strip $(INCLUDE_CPU_AR368FE)),y)
CPU_TYPE=ar368fe
endif

ifeq ($(strip $(INCLUDE_CPU_RT63368)),y)
CPU_TYPE=rt63368
endif

ifeq ($(strip $(INCLUDE_CPU_RT63365)),y)
CPU_TYPE=rt63365
endif

ifeq ($(strip $(INCLUDE_CPU_88F6550)),y)
CPU_TYPE=88f6550
include config/config.wifi
endif

ifeq ($(strip $(INCLUDE_CPU_MT7513)),y)
CPU_TYPE=en7512
endif

#define basic variable
TOP_PATH = $(shell pwd)/..
BUILD_PATH = $(TOP_PATH)/build
TOOLS_PATH = $(TOP_PATH)/host_tools
FAKEROOT_PATH = $(TOOLS_PATH)/fakeroot

#Temporarily
ifeq ($(strip $(INCLUDE_LANGUAGE_EN)),y)
ifeq ($(strip $(INCLUDE_PROWARE_WEB)),y)
WEBFILE_PATH = $(TOP_PATH)/sysfiles/web/en_proware
endif
ifeq ($(strip $(INCLUDE_TPLINK_WEB)),y)
WEBFILE_PATH = $(TOP_PATH)/sysfiles/web/en_tplink
endif
CONFFILE_PATH = $(TOP_PATH)/sysfiles/config/en
endif

ifeq ($(strip $(INCLUDE_LANGUAGE_CN)),y)
WEBFILE_PATH = $(TOP_PATH)/sysfiles/web/cn
CONFFILE_PATH = $(TOP_PATH)/sysfiles/config/cn
endif

TARGETS_PATH = $(TOP_PATH)/targets
PUBLIC_APPS_PATH = $(TOP_PATH)/apps/public
PRIVATE_APPS_PATH = $(TOP_PATH)/apps/private
OS_LIB_PATH = $(PUBLIC_APPS_PATH)/os_libs
TP_MODULES_PATH = $(TOP_PATH)/modules/private

ifeq ($(INCLUDE_PORTABLE_APP), y)
JSON_INOUT_PATH = $(WEBFILE_PATH)/common/js
endif

####################################################
#
#Get supplier         
#
####################################################	
ifeq ($(strip $(CPU_TYPE)),88f6550)
#ChenZhu add 20130402 for bigendian is not supported
export BIGENDIAN = 0
SUPPLIER = environment_marvel88f6560
SUPPLIER_MARVELL = environment_marvel88f6560
export ARCH = arm
export KERNELARCH = arm
TOOLROOTPATH :=/opt/marvell/armv5-marvell-linux-uclibcgnueabi-soft_i686
export CROSS_COMPILE = $(TOOLROOTPATH)/bin/arm-marvell-linux-uclibcgnueabi-
export PATH := $(TOOLROOTPATH)/bin:$(PATH)
TOOLPREFIX = arm-marvell-linux-uclibcgnueabi-
HOST = arm-marvell-linux-uclibcgnueabi
KERNELVERSION = 2.6.32.11
OBJ_DIR = marvell_2.6.25-RC10
export KERNELPATH = $(TOP_PATH)/$(SUPPLIER)/SDK_2.6.25-RC10/Source/Kernel/linux_feroceon-KW2

AVANTA_HGU_CFG_DIR = $(TOP_PATH)/$(SUPPLIER)/SDK_2.6.25-RC10/Source/ConfigFiles/Products/AVANTA_HGU
AVANTA_HGU_INITD_DIR = $(AVANTA_HGU_CFG_DIR)/FileSystem/init.d

KERNEL_CFG_DIR = $(AVANTA_HGU_CFG_DIR)/Kernel

MARVELL_KERNEL_IMAGE_DIR = $(KERNELPATH)/arch/arm/boot
MARVELL_KERNEL_IMAGE_NAME = uImage

MARVELL_SQAUSH_FS_DIR = $(TOP_PATH)/$(SUPPLIER)/SDK_2.6.25-RC10/Source
MARVELL_SQAUSH_FS_NAME = rootfs.squashfs
MARVELL_JFFS2_FS_NAME = jffs2.image

MARVELL_IMAGE_DIR = $(TOP_PATH)/$(SUPPLIER)/SDK_2.6.25-RC10/Output/Products/AVANTA_HGU/RecoveryImage
MARVELL_IMAGE_NAME = rootfs.squashfs.rcvr.img

export MAKEARCH = $(MAKE) ARCH=$(KERNELARCH) CROSS_COMPILE=$(TOOLPREFIX)

export HAL = $(TOP_PATH)/$(SUPPLIER)/modules/private/wlan/hal
export TARGETARCH = xscale-le-elf
export MODULEPATH = $(TARGETS_PATH)/fs.$(MODEL)/lib/modules
WIRELESSTOOLS = wireless_tools.29
WIFI_TOOL_TG_PATH = $(TARGETS_PATH)/$(SUPPLIER)_$(CPU_TYPE)_wifi_tool
WIFI_MODULE_TG_PATH = $(TARGETS_PATH)/$(SUPPLIER)_$(CPU_TYPE)_wifi_modules
INSTALL_WIRELESS = $(WIFI_TOOL_TG_PATH)
FS_WIRELESS_MODULE = $(MODULEPATH)
FS_WIRELESS_TOOLS = $(TARGETS_PATH)/fs.$(MODEL)/usr
WIRELESSTOOLSLIB = iwlib.so

export WIFI_MODULE_TG_PATH

#export ATH_CARR_DIR = marvel
export PLATFORM = $(TOP_PATH)/environment_marvel88f6560/modules/private/wlan
#export PLATFORM_INC = $(PLATFORM)/carrier/$(ATH_CARR_DIR)/include
export ATHEROSPATH=$(PLATFORM)

export MARVELL_SW_DIR = $(TOP_PATH)/$(SUPPLIER)/SDK_2.6.25-RC10/Source/Application/SW_TREE_PUB
#export MARVELL_SW_LIBS_INC = -I$(MARVELL_SW_DIR)/mng/i2c/appl/i2c_app/inc
export MARVELL_SW_LIBS_INC = -I$(MARVELL_SW_DIR)/mng/omci/core/h	#Added by YeZuopou @ 01Apr13
export MARVELL_SW_LIBS_LIB = -L$(MARVELL_SW_DIR)/mipc/obj -lmipc \
							-L$(MARVELL_SW_DIR)/mipc/api_defs/omci -lomci_mipc_client \
							-lm 
endif

ifeq ($(strip $(CPU_TYPE)),tc3182)
TOOLROOTPATH :=/opt/trendchip/mips-linux-uclibc/usr
export PATH :=$(TOOLROOTPATH)/bin:$(PATH)
SUPPLIER = ralink
TOOLPREFIX = mips-linux-uclibc-
HOST = mips-linux-uclibc
KERNELVERSION = 2.6.22.15
OBJ_DIR = ralink_4.3.4
SLIB_PATH = $(TOP_PATH)/$(SUPPLIER)/Project/lib/mips-linux-uclibc/
KERNELPATH = $(TOP_PATH)/$(SUPPLIER)/linux
ROOTFSTOOLS = $(SUPPLIER_TOOLS)/mksquashfs-lzma

ifeq ($(strip $(SUPPLIER)),ralink)

unexport TC_CFLAGS
ifeq ($(INCLUDE_VOIP), y)
include voip.mak
TC_CFLAGS += $(VOIP_LOCALE)
ifeq ($(INCLUDE_DSP_SOCKET_OPEN), y)
TC_CFLAGS += -DINCLUDE_DSP_SOCKET_OPEN
endif 
endif

ifeq ($(strip $(TCSUPPORT_SDRAM_32M)),y)
TC_CFLAGS += -DTCSUPPORT_SDRAM_32M
endif

ifeq ($(INCLUDE_WATCHDOG), y) 
TC_CFLAGS += -DTCSUPPORT_WATCHDOG
endif

ifeq ($(strip $(TCSUPPORT_BRIDGE_FASTPATH)),y)
TC_CFLAGS += -DTCSUPPORT_BRIDGE_FASTPATH
endif
export TC_CFLAGS
endif

endif

ifeq ($(strip $(CPU_TYPE)),tc3162u)
export PATH :=/opt/trendchip/mips-linux-3.4.6/bin:$(PATH)
SUPPLIER = ralink
TOOLPREFIX = mips-linux-
HOST = mips-linux
KERNELVERSION = 2.6.22.15
OBJ_DIR = ralink_3.4.6
SLIB_PATH = $(TOP_PATH)/$(SUPPLIER)/Project/lib/mips-linux/
KERNELPATH = $(TOP_PATH)/$(SUPPLIER)/linux
ROOTFSTOOLS = $(SUPPLIER_TOOLS)/mksquashfs-lzma
endif

#zl added 2011-10-18
ifeq ($(strip $(CPU_TYPE)),rtl8672)
export PATH :=/opt/rtl/rsdk-1.3.6-5181-EB-2.6.30-0.9.30/bin:$(PATH)
SUPPLIER = realtek
TOOLPREFIX = rsdk-linux-
HOST = rsdk-linux
KERNELVERSION = 2.6.30.9
OBJ_DIR = realtek_1.3.6
SLIB_PATH = $(TOP_PATH)/$(SUPPLIER)/vendors/rsdk-linux/
KERNELPATH = $(TOP_PATH)/$(SUPPLIER)/linux-2.6.x
ROOTFSTOOLS = $(KERNELPATH)/mksquashfs
#For lzma when make rootfs
export PATH := $(KERNELPATH)/rtkload/:$(PATH)
endif
#end--added

ifeq ($(strip $(CPU_TYPE)),vr268)
SUPPLIER = lantiqVR268
TOOLPREFIX = mips-openwrt-linux-uclibc-
HOST = mips-openwrt-linux-uclibc
KERNELVERSION = 2.6.32.42
OBJ_DIR = lantiq_4.3.3
SLIB_PATH = $(TOP_PATH)/$(SUPPLIER)/build_dir/target-mips_r2_uClibc-0.9.30.1_AC750/root-ltqcpe/orilib
export KERNELPATH = $(TOP_PATH)/$(SUPPLIER)/build_dir/linux-ltqcpe_AC750/linux-2.6.32.42
ROOTFSTOOLS = $(TOP_PATH)/$(SUPPLIER)/staging_dir/host/bin/mksquashfs4
TOOLCHAIN_DIR = $(TOP_PATH)/$(SUPPLIER)/staging_dir/toolchain-mips_r2_gcc-4.3.3+cs_uClibc-0.9.30.1
export PATH :=$(TOP_PATH)/$(SUPPLIER)/staging_dir/toolchain-mips_r2_gcc-4.3.3+cs_uClibc-0.9.30.1/usr/bin:$(PATH)
export PATH :=$(TOP_PATH)/$(SUPPLIER)/staging_dir/host/bin:$(PATH)
export HAL = $(TOP_PATH)/$(SUPPLIER)/modules/private/wlan/hal
export KERNELARCH = mips
export MAKEARCH = $(MAKE) ARCH=$(KERNELARCH) CROSS_COMPILE=$(TOOLPREFIX)
export ATH_CARR_DIR = lantiq
export PLATFORM = $(TOP_PATH)/lantiq/modules/private/wlan
export PLATFORM_INC = $(PLATFORM)/carrier/$(ATH_CARR_DIR)/include
export WAVE300_DIR = $(TOP_PATH)/lantiq/modules/wave300
export INSTALL_WIRELESS = $(TOP_PATH)/$(SUPPLIER)/wireless
export MODULEPATH = $(INSTALL_WIRELESS)/lib/modules/
export ATHEROSPATH=$(PLATFORM)

LANTIQ_ROOT_PATH=$(TOP_PATH)/$(SUPPLIER)/build_dir/target-mips_r2_uClibc-0.9.30.1_AC750/root-ltqcpe
endif

# wzy added 2013-7-17
ifeq ($(strip $(CPU_TYPE)),vr288)
SUPPLIER = lantiq
TOOLPREFIX = mips-openwrt-linux-uclibc-
HOST = mips-openwrt-linux-uclibc
KERNELVERSION = 2.6.32.42
OBJ_DIR = lantiq_4.3.3
SLIB_PATH = $(TOP_PATH)/$(SUPPLIER)/build_dir/target-mips_r2_uClibc-0.9.30.1_AC750/root-ltqcpe/orilib
export KERNELPATH = $(TOP_PATH)/$(SUPPLIER)/build_dir/linux-ltqcpe_AC750/linux-2.6.32.42
ROOTFSTOOLS = $(TOP_PATH)/$(SUPPLIER)/staging_dir/host/bin/mksquashfs4
TOOLCHAIN_DIR = $(TOP_PATH)/$(SUPPLIER)/staging_dir/toolchain-mips_r2_gcc-4.3.3+cs_uClibc-0.9.30.1
export PATH :=$(TOP_PATH)/$(SUPPLIER)/staging_dir/toolchain-mips_r2_gcc-4.3.3+cs_uClibc-0.9.30.1/usr/bin:$(PATH)
export PATH :=$(TOP_PATH)/$(SUPPLIER)/staging_dir/host/bin:$(PATH)
# export HAL = $(TOP_PATH)/$(SUPPLIER)/modules/private/wlan/hal
export KERNELARCH = mips
export MAKEARCH = $(MAKE) ARCH=$(KERNELARCH) CROSS_COMPILE=$(TOOLPREFIX)
export ATH_CARR_DIR = lantiq
# export PLATFORM = $(TOP_PATH)/lantiq/modules/private/wlan
# export PLATFORM_INC = $(PLATFORM)/carrier/$(ATH_CARR_DIR)/include
export WAVE300_DIR = $(TOP_PATH)/lantiq/modules/wave300
export MT7610E_DIR = $(TOP_PATH)/lantiq/modules/private/mt7610e_wifi
export INSTALL_WIRELESS = $(TOP_PATH)/$(SUPPLIER)/wireless
export MODULEPATH = $(INSTALL_WIRELESS)/lib/modules/
# export ATHEROSPATH=$(PLATFORM)
export WIRELESSTOOLS=wireless_tools.29
export WIRELESSTOOLSLIB=libiw.so.29

LANTIQ_ROOT_PATH=$(TOP_PATH)/$(SUPPLIER)/build_dir/target-mips_r2_uClibc-0.9.30.1_AC750/root-ltqcpe
endif
# end--added

ifeq ($(strip $(CPU_TYPE)),rt63368)
TOOLROOTPATH :=/opt/trendchip/mips-linux-uclibc/usr
export PATH :=$(TOOLROOTPATH)/bin:$(PATH)
SUPPLIER = mtk
TOOLPREFIX = mips-linux-uclibc-
HOST = mips-linux-uclibc
KERNELVERSION = 2.6.36
OBJ_DIR = mtk_4.3.4
SLIB_PATH = $(TOP_PATH)/$(SUPPLIER)/Project/lib/mips-linux-uclibc/
KERNELPATH = $(TOP_PATH)/$(SUPPLIER)/linux-2.6.36
ROOTFSTOOLS = $(SUPPLIER_TOOLS)/mksquashfs4

unexport TC_CFLAGS

ifeq ($(INCLUDE_VOIP), y)
include voip.mak
TC_CFLAGS += -DTCSUPPORT_VOIP $(VOIP_LOCALE)
ifeq ($(INCLUDE_DSP_SOCKET_OPEN), y)
TC_CFLAGS += -DINCLUDE_DSP_SOCKET_OPEN
endif 
endif

ifeq ($(INCLUDE_WATCHDOG), y) 
TC_CFLAGS += -DTCSUPPORT_WATCHDOG
endif

export TC_CFLAGS
export KERNELVERSION

endif

ifeq ($(strip $(CPU_TYPE)),rt63365)
TOOLROOTPATH :=/opt/trendchip/mips-linux-uclibc/usr
export PATH :=$(TOOLROOTPATH)/bin:$(PATH)
SUPPLIER = mtk
TOOLPREFIX = mips-linux-uclibc-
HOST = mips-linux-uclibc
KERNELVERSION = 2.6.36
OBJ_DIR = mtk_4.3.4
SLIB_PATH = $(TOP_PATH)/$(SUPPLIER)/Project/lib/mips-linux-uclibc/
KERNELPATH = $(TOP_PATH)/$(SUPPLIER)/linux-2.6.36
ROOTFSTOOLS = $(SUPPLIER_TOOLS)/mksquashfs4


unexport TC_CFLAGS
TC_CFLAGS += -DTCSUPPORT_IGMPSNOOPING_ENHANCE
export TC_CFLAGS

export KERNELVERSION

endif

#add by dbs
ifeq ($(strip $(CPU_TYPE)), en7512)
TOOLROOTPATH :=/opt/trendchip/mips-linux-uclibc-4.3.6-v2/usr
export PATH :=$(TOOLROOTPATH)/bin:$(PATH)
SUPPLIER = mtk
TOOLPREFIX = mips-unknown-linux-uclibc-
HOST = mips-linux-uclibc
CROSS_COMPILE = mips-unknown-linux-uclibc-
export KERNELARCH = mips
KERNELVERSION = 2.6.36
ifeq ($(INCLUDE_ISP_M2), y)
OBJ_DIR = pvw422t1200_isp_m2
else
OBJ_DIR = mtk_4.3.4
endif
SLIB_PATH = $(TOP_PATH)/$(SUPPLIER)/Project/lib/mips-unknown-linux-uclibc/
KERNELPATH = $(TOP_PATH)/$(SUPPLIER)/linux-2.6.36
ROOTFSTOOLS = $(SUPPLIER_TOOLS)/mksquashfs4

unexport TC_CFLAGS
TC_CFLAGS += -DTCSUPPORT_IGMPSNOOPING_ENHANCE
TC_CFLAGS += -DTCSUPPORT_CPU_EN7512
export TC_CFLAGS
export TP_CFLAGS

export KERNELVERSION

PROFILE_PATH = $(TOP_PATH)/$(SUPPLIER)/Project/profile/RT/$(CPU_TYPE)_demo/
#copy model profile as en7512_demo.profile
COPY_PROFILE := $(shell cp -f $(PROFILE_PATH)/$(MODEL).profile $(PROFILE_PATH)/$(CPU_TYPE)_demo.profile)
export DMT_PATH = $(TOP_PATH)/$(SUPPLIER)/modules/private/dmt/7512
export SYSROOT = /opt/trendchip/mips-linux-uclibc-4.3.6-v2/usr/mips-unknown-linux-uclibc/sysroot

include $(PROFILE_PATH)/$(CPU_TYPE)_demo.profile
include $(TOP_PATH)/$(SUPPLIER)/Project/rule.mak

#add by Zhao Mengqing, 2016-8-3, when build kernel, we need these path
MODULES_RA_HWNAT_7510_DIR = $(TOP_PATH)/$(SUPPLIER)/modules/private/ra_hwnat_7510
GLOBAL_INC_DIR = $(TOP_PATH)/$(SUPPLIER)/global_inc
endif

# yangxv added 2013-12
# ifeq ($(strip $(CPU_TYPE)),ar368)
ifeq ($(strip $(CPU_TYPE)),$(filter $(CPU_TYPE), ar368 ar368fe))

ifeq ($(strip $(CPU_TYPE)),ar368)
SUPPLIER = lantiqAR368
endif #ar368
ifeq ($(strip $(CPU_TYPE)),ar368fe)
SUPPLIER = lantiqAR368FE
endif #ar368fe

TOOLPREFIX = mips-openwrt-linux-uclibc-
HOST = mips-openwrt-linux-uclibc
KERNELVERSION = 2.6.32.42
OBJ_DIR = lantiq_4.3.3
SLIB_PATH = $(TOP_PATH)/$(SUPPLIER)/build_dir/target-mips_r2_uClibc-0.9.30.1_AC750/root-ltqcpe/orilib
export KERNELPATH = $(TOP_PATH)/$(SUPPLIER)/build_dir/linux-ltqcpe_AC750/linux-2.6.32.42
ROOTFSTOOLS = $(TOP_PATH)/$(SUPPLIER)/staging_dir/host/bin/mksquashfs4
TOOLCHAIN_DIR = $(TOP_PATH)/$(SUPPLIER)/staging_dir/toolchain-mips_r2_gcc-4.3.3+cs_uClibc-0.9.30.1
export PATH :=$(TOP_PATH)/$(SUPPLIER)/staging_dir/toolchain-mips_r2_gcc-4.3.3+cs_uClibc-0.9.30.1/usr/bin:$(PATH)
export PATH :=$(TOP_PATH)/$(SUPPLIER)/staging_dir/host/bin:$(PATH)
export KERNELARCH = mips
export MAKEARCH = $(MAKE) ARCH=$(KERNELARCH) CROSS_COMPILE=$(TOOLPREFIX)
export ATH_CARR_DIR = lantiq
export WAVE300_DIR = $(TOP_PATH)/lantiq/modules/wave300
export MT7610E_DIR = $(TOP_PATH)/lantiq/modules/private/mt7610e_wifi
export INSTALL_WIRELESS = $(TOP_PATH)/$(SUPPLIER)/wireless
export MODULEPATH = $(INSTALL_WIRELESS)/lib/modules/
export WIRELESSTOOLS=wireless_tools.29
export WIRELESSTOOLSLIB=libiw.so.29

LANTIQ_ROOT_PATH=$(TOP_PATH)/$(SUPPLIER)/build_dir/target-mips_r2_uClibc-0.9.30.1_LANTIQ_D2/root-ltqcpe
endif
# end--ar368

####################################################
#
#Supplier PATH variable        
#
####################################################	

INSTALL_MOD_PATH = $(KERNELPATH)/../
export MODULES_PATH = $(TOP_PATH)/$(SUPPLIER)/modules/private
SUPPLIER_TOOLS = $(TOP_PATH)/$(SUPPLIER)/tools

ifeq ($(strip $(SUPPLIER)),realtek)
ifeq ($(INCLUDE_LAN_WLAN), y)
WIRELESS_TOOLS = iwpriv
endif
endif

#TG means target
BOOT_TG_PATH = $(TARGETS_PATH)/$(SUPPLIER)_boot
KERNEL_TG_PATH = $(TARGETS_PATH)/$(SUPPLIER)_$(CPU_TYPE)_kernel
MODULE_TG_PATH = $(TARGETS_PATH)/$(SUPPLIER)_$(CPU_TYPE)_modules
MODEL_FS_PATH = $(TARGETS_PATH)/fs.$(MODEL)
TOOLS_FS_PATH = $(TARGETS_PATH)/$(SUPPLIER)_$(CPU_TYPE)_tools

PRIVATE_LIB_BIN_PATH = $(PRIVATE_APPS_PATH)/bins/$(OBJ_DIR)/libs/
PRIVATE_APPS_BIN_PATH = $(PRIVATE_APPS_PATH)/bins/$(OBJ_DIR)/apps/

####################################################
#
#Supplier compiler variable        
#
####################################################	

ifeq ($(INCLUDE_VOIP), y)
sinclude voip.mak
endif

CC = $(TOOLPREFIX)gcc
LD = $(TOOLPREFIX)ld
AR = $(TOOLPREFIX)ar
STRIP = $(TOOLPREFIX)strip
RANLIB = $(TOOLPREFIX)ranlib

####################################################
#
#PJSIP Variable
#
####################################################
#PJSIP_CONFCACHE_EXIST = $(wildcard $(PUBLIC_APPS_PATH)/pjsip_1.10/config_cache_file) 
PJSIP_CONFCACHE_EXIST =

####################################################
#
#Function conditional compilation        
#
####################################################	

DFLAGS = 
WEBFLAGS = 
DF_FLAGS = 

ifeq ($(INCLUDE_CPU_BIG_ENDIAN), y)
DFLAGS += -DINCLUDE_CPU_BIG_ENDIAN
CMM_TARGET_CPU_ENDIAN = 0
export CMM_TARGET_CPU_ENDIAN
endif

ifeq ($(INCLUDE_CPU_LITTLE_ENDIAN), y)
DFLAGS += -DINCLUDE_CPU_LITTLE_ENDIAN
CMM_TARGET_CPU_ENDIAN = 1
export CMM_TARGET_CPU_ENDIAN
endif

ifeq ($(INCLUDE_CPU_AR368), y) 
DFLAGS += -DINCLUDE_CPU_AR368
endif 

ifeq ($(INCLUDE_CPU_AR368FE), y) 
DFLAGS += -DINCLUDE_CPU_AR368FE
endif 

ifeq ($(strip $(INCLUDE_SOCKET_LOCK)), y)
DFLAGS += -DINCLUDE_SOCKET_LOCK
export INCLUDE_SOCKET_LOCK=y
endif

ifeq ($(INCLUDE_SERBIA), y) 
DFLAGS += -DINCLUDE_SERBIA
endif 

ifeq ($(INCLUDE_WATCHDOG), y) 
DFLAGS += -DINCLUDE_WATCHDOG
endif 

ifeq ($(INCLUDE_PPA_ACCELERATE), y)
DFLAGS += -DINCLUDE_PPA_ACCELERATE
endif

ifeq ($(INCLUDE_MTK_ACCELERATE), y)
DFLAGS += -DINCLUDE_MTK_ACCELERATE
endif

ifeq ($(INCLUDE_MTD_TYPE1), y)
DFLAGS += -DINCLUDE_MTD_TYPE1
MTD_PART_TYPE = 1
endif

ifeq ($(INCLUDE_MTD_TYPE2), y)
DFLAGS += -DINCLUDE_MTD_TYPE2
MTD_PART_TYPE = 2
endif

ifeq ($(INCLUDE_MTD_TYPE3), y)
DFLAGS += -DINCLUDE_MTD_TYPE3
MTD_PART_TYPE = 3
endif

ifeq ($(INCLUDE_MTD_TYPE4), y)
DFLAGS += -DINCLUDE_MTD_TYPE4
MTD_PART_TYPE = 4
endif

ifeq ($(INCLUDE_MTD_TYPE5), y)
DFLAGS += -DINCLUDE_MTD_TYPE5
MTD_PART_TYPE = 5
endif

ifeq ($(INCLUDE_MTD_TYPE6), y)
DFLAGS += -DINCLUDE_MTD_TYPE6
MTD_PART_TYPE = 6
endif

ifeq ($(INCLUDE_FLASH_8M), y)
DFLAGS += -DINCLUDE_FLASH_8M
FLASH_SIZE = 0x800000
TP_CFLAGS += -DDEFAULT_FLASH_SIZE=$(FLASH_SIZE)
endif

ifeq ($(INCLUDE_FLASH_16M), y)
DFLAGS += -DINCLUDE_FLASH_16M
FLASH_SIZE = 0x1000000
TP_CFLAGS += -DDEFAULT_FLASH_SIZE=$(FLASH_SIZE)
endif

ifeq ($(INCLUDE_CLI_FULL_FEATURE), y)
DFLAGS += -DINCLUDE_CLI_FULL_FEATURE
endif

ifeq ($(INCLUDE_USER_ACCOUNT), y) 
DFLAGS += -DINCLUDE_USER_ACCOUNT
endif

ifeq ($(INCLUDE_CHGINIT_PWD), y) 
DFLAGS += -DINCLUDE_CHGINIT_PWD
endif

ifeq ($(INCLUDE_OPTION66), y)
DFLAGS += -DINCLUDE_OPTION66
ifeq ($(INCLUDE_OPTION66_NEED_TAG), y)
DFLAGS += -DINCLUDE_OPTION66_NEED_TAG
endif
endif

ifeq ($(INCLUDE_CHGINIT_PWD_CN), y) 
DFLAGS += -DINCLUDE_CHGINIT_PWD_CN
endif

ifeq ($(INCLUDE_AUTH_PASSWORD), y) 
DFLAGS += -DINCLUDE_AUTH_PASSWORD
endif

export BASED_MODEL=$(subst ",,$(INCLUDE_BASED_MODEL))
ifneq ($(BASED_MODEL), )
DFLAGS += -DINCLUDE_$(BASED_MODEL)
endif 

ifneq ($(INCLUDE_CUSTOMIZED_SP), "")
export CSP=$(subst ",,$(INCLUDE_CUSTOMIZED_SP))
DFLAGS += -DINCLUDE_$(CSP)
endif 

ifeq ($(INCLUDE_AUTO_GATEWAY), y)
DFLAGS += -DINCLUDE_AUTO_GATEWAY
endif

#################### ETH Type ######################
ifeq ($(INCLUDE_ETH_LTQ), y) 
DFLAGS += -DINCLUDE_ETH_LTQ
endif 

ifeq ($(INCLUDE_ETH_RA), y) 
DFLAGS += -DINCLUDE_ETH_RA
endif 

ifeq ($(INCLUDE_ETH_ATH), y) 
DFLAGS += -DINCLUDE_ETH_ATH
endif 

ifeq ($(INCLUDE_GMAC1_ONLY), y)
DFLAGS += -DINCLUDE_GMAC1_ONLY
endif

###################### WIFI  ########################
ifeq ($(INCLUDE_LAN_WLAN), y) 
DFLAGS += -DINCLUDE_LAN_WLAN 
endif 

ifeq ($(INCLUDE_WLAN_RA), y) 
DFLAGS += -DINCLUDE_WLAN_RA
endif 

ifeq ($(INCLUDE_WLAN_RTL), y) 
DFLAGS += -DINCLUDE_WLAN_RTL
endif 

ifeq ($(INCLUDE_WLAN_ATH), y) 
DFLAGS += -DINCLUDE_WLAN_ATH
endif 

ifeq ($(ATH_A_BAND_MIMO_NUM_3_3), y)
DFLAGS += -DWLAN_ATH_A_BAND_MIMO_NUM=3
endif

ifeq ($(ATH_G_BAND_MIMO_NUM_2_2), y)
DFLAGS += -DWLAN_ATH_G_BAND_MIMO_NUM=2
endif

ifeq ($(ATH_G_BAND_MIMO_NUM_3_3), y)
DFLAGS += -DWLAN_ATH_G_BAND_MIMO_NUM=3
endif

ifeq ($(INCLUDE_LAN_WLAN_MSSID), y) 
DFLAGS += -DINCLUDE_LAN_WLAN_MSSID 
endif 

ifdef WLAN_VAP_NUM
DFLAGS += -DWLAN_VAP_NUM=$(WLAN_VAP_NUM)
endif

ifeq ($(INCLUDE_LAN_WLAN_GUESTNETWORK), y) 
DFLAGS += -DINCLUDE_LAN_WLAN_GUESTNETWORK 
endif 

ifeq ($(INCLUDE_LAN_WLAN_GUESTNETWORK), y) 
DFLAGS += -DINCLUDE_HIDE_GUESTNETWORK 
endif 

ifeq ($(INCLUDE_LAN_WLAN_THREE_PPA), y) 
DFLAGS += -DINCLUDE_LAN_WLAN_THREE_PPA
endif 

ifeq ($(INCLUDE_LAN_WLAN_WDS), y) 
DFLAGS += -DINCLUDE_LAN_WLAN_WDS
endif 

ifeq ($(INCLUDE_LAN_WLAN_WDS_DETECT), y)
DFLAGS += -DINCLUDE_LAN_WLAN_WDS_DETECT
endif

ifeq ($(INCLUDE_LAN_WLAN_8021X), y) 
DFLAGS += -DINCLUDE_LAN_WLAN_802_1X
endif 

ifeq ($(INCLUDE_LAN_WLAN_DUALBAND),y)
DFLAGS += -DINCLUDE_LAN_WLAN_DUALBAND
endif

ifeq ($(INCLUDE_LAN_WLAN_AC),y)
DFLAGS+= -DINCLUDE_LAN_WLAN_AC
endif

ifeq ($(INCLUDE_LAN_WLAN_SCHEDULE), y)
DFLAGS += -DINCLUDE_LAN_WLAN_SCHEDULE
endif

#added by xw 2013_9_9
ifeq ($(INCLUDE_WLAN_LQ), y)
DFLAGS += -DINCLUDE_WLAN_LQ

ifeq ($(INCLUDE_WLAN_LQ_WRX313), y)
DFLAGS += -DINCLUDE_WLAN_LQ_WRX313
endif

ifeq ($(INCLUDE_WLAN_LQ_WAVE300), y)
DFLAGS += -DINCLUDE_WLAN_LQ_WAVE300
endif


endif


ifeq ($(INCLUDE_LAN_WLAN_DIFFERENT_CHIP),y)
DFLAGS += -DINCLUDE_LAN_WLAN_DIFFERENT_CHIP
endif

ifeq ($(INCLUDE_WIFI_5G_CHIP_MT7612E), y)
DFLAGS += -DINCLUDE_WIFI_5G_CHIP_MT7612E
else ifeq ($(INCLUDE_WIFI_5G_CHIP_MT7615E), y)
DFLAGS += -DINCLUDE_WIFI_5G_CHIP_MT7615E
endif 

ifeq ($(INCLUDE_ANTI_INTERFERENCE), y)
DFLAGS += -DINCLUDE_ANTI_INTERFERENCE
endif

###################### PON  ########################

ifeq ($(strip $(INCLUDE_CPU_88F6550)),y)
DFLAGS += -DINCLUDE_PON_VG1530_ENV
DFLAGS += -DINCLUDE_PON_MARVELL_SW_LIB
endif

ifeq ($(INCLUDE_PON), y) 
DFLAGS += -DINCLUDE_PON
endif 

ifeq ($(INCLUDE_PON_EPON), y) 
DFLAGS += -DINCLUDE_PON_EPON
endif 

ifeq ($(INCLUDE_PON_GPON), y)
DFLAGS += -DINCLUDE_PON_GPON
endif

#################### Function ######################

ifeq ($(INCLUDE_BRIDGING), y)
DFLAGS += -DINCLUDE_BRIDGING
endif

ifeq ($(INCLUDE_ADSLWAN), y)
DFLAGS += -DINCLUDE_ADSLWAN
endif

ifeq ($(INCLUDE_VDSLWAN), y)
DFLAGS += -DINCLUDE_VDSLWAN
endif

ifeq ($(INCLUDE_VDSL_RA), y)
DFLAGS += -DINCLUDE_VDSL_RA
endif

ifeq ($(INCLUDE_ADSL_RA), y)
DFLAGS += -DINCLUDE_ADSL_RA
endif

ifeq ($(INCLUDE_ADSL_RTL), y)
DFLAGS += -DINCLUDE_ADSL_RTL
endif

ifeq ($(INCLUDE_ADSL_LTQ), y)
DFLAGS += -DINCLUDE_ADSL_LTQ
endif

ifeq ($(INCLUDE_ETHERNET_WAN), y)
DFLAGS += -DINCLUDE_ETHERNET_WAN
endif

ifeq ($(INCLUDE_ETHERNET_WAN_VLAN), y)
DFLAGS += -DINCLUDE_ETHERNET_WAN_VLAN
endif

ifeq ($(INCLUDE_ETHERNET_MULTI_VLAN), y)
DFLAGS += -DINCLUDE_ETHERNET_MULTI_VLAN
endif

ifeq ($(INCLUDE_IGMP), y)
DFLAGS += -DINCLUDE_IGMP
endif

ifeq ($(INCLUDE_IPTV), y)
DFLAGS += -DINCLUDE_IPTV
endif

ifeq ($(INCLUDE_AUTO_PVC), y)
DFLAGS += -DINCLUDE_AUTO_PVC
endif

ifeq ($(INCLUDE_RIP), y)
DFLAGS += -DINCLUDE_RIP
endif

ifeq ($(INCLUDE_SNMP), y)
DFLAGS += -DINCLUDE_SNMP
endif

ifeq ($(INCLUDE_SSL), y)
DFLAGS += -DINCLUDE_SSL
endif

ifeq ($(INCLUDE_CWMP), y)
DFLAGS += -DINCLUDE_CWMP
endif

ifeq ($(INCLUDE_CWMP_SSL), y)
DFLAGS += -DINCLUDE_CWMP_SSL
export INCLUDE_CWMP_SSL
ifeq ($(INCLUDE_CWMP_SSL_VERIFY_PEER), y)
DFLAGS += -DINCLUDE_CWMP_SSL_VERIFY_PEER
export INCLUDE_CWMP_SSL_VERIFY_PEER
endif
endif

ifeq ($(INCLUDE_DIGITAL_SIGNATURE), y)
DFLAGS += -DINCLUDE_DIGITAL_SIGNATURE
endif

ifeq ($(INCLUDE_DDNS), y)
DFLAGS += -DINCLUDE_DDNS
endif

ifeq ($(INCLUDE_DDNS_PH), y)
DFLAGS += -DINCLUDE_DDNS_PH
endif

ifeq ($(INCLUDE_DYNDNS), y)
DFLAGS += -DINCLUDE_DYNDNS
endif

ifeq ($(INCLUDE_NOIPDNS), y)
DFLAGS += -DINCLUDE_NOIPDNS
endif

ifeq ($(INCLUDE_DDNS_USERDEFINE), y)
DFLAGS += -DINCLUDE_DDNS_USERDEFINE

ifeq ($(INCLUDE_DDNS_USERDEFINE_DETAIL), y)
DFLAGS += -DINCLUDE_DDNS_USERDEFINE_DETAIL
endif

endif

ifeq ($(INCLUDE_ALG), y)
DFLAGS += -DINCLUDE_ALG
endif

ifeq ($(INCLUDE_ALG_H323), y)
DFLAGS += -DINCLUDE_ALG_H323
endif

ifeq ($(INCLUDE_ALG_SIP), y)
DFLAGS += -DINCLUDE_ALG_SIP
endif

ifeq ($(INCLUDE_WAN_DETECT), y)
DFLAGS += -DINCLUDE_WAN_DETECT
endif

ifeq ($(INCLUDE_APP_IQOS), y)
DFLAGS += -DINCLUDE_APP_IQOS
endif

#NOTE:INCLUDE_VOIP in voip.mak
ifeq ($(INCLUDE_VOIP), y)
DFLAGS += $(VOIP_DFLAGS)
DF_FLAGS += $(VOIP_CFLAGS)
endif

ifeq ($(INCLUDE_IPV6), y)
DFLAGS += -DINCLUDE_IPV6

ifeq ($(INCLUDE_IPV6_MLD), y)
DFLAGS += -DINCLUDE_IPV6_MLD
endif

ifeq ($(INCLUDE_IPV6_SLAAC), y)
DFLAGS += -DINCLUDE_IPV6_SLAAC
endif

ifeq ($(INCLUDE_IPV6_HTTP), y)
DFLAGS += -DINCLUDE_IPV6_HTTP
endif

ifeq ($(INCLUDE_IPV6_TUNNEL_GATEWAY), y)
DFLAGS += -DINCLUDE_IPV6_TUNNEL_GATEWAY
WEBFLAGS += INCLUDE_IPV6_TUNNEL_GATEWAY=1
else
WEBFLAGS += INCLUDE_IPV6_TUNNEL_GATEWAY=0
endif

endif

ifeq ($(INCLUDE_USB), y)
DFLAGS += -DINCLUDE_USB
endif

ifeq ($(INCLUDE_USB_OVER_IP), y)
DFLAGS += -DINCLUDE_USB_OVER_IP
endif

ifeq ($(INCLUDE_USB_OVER_IP_TPLINK), y)
DFLAGS += -DINCLUDE_USB_OVER_IP_TPLINK
endif

ifeq ($(INCLUDE_USB_OVER_IP_KCODES), y)
DFLAGS += -DINCLUDE_USB_OVER_IP_KCODES
endif

ifeq ($(INCLUDE_USB_STORAGE), y)
DFLAGS += -DINCLUDE_USB_STORAGE
endif

ifeq ($(INCLUDE_USB_MEDIA_SERVER), y)
DFLAGS += -DINCLUDE_USB_MEDIA_SERVER
endif

ifeq ($(INCLUDE_USHARE), y)
DFLAGS += -DINCLUDE_USHARE
endif

ifeq ($(INCLUDE_MINIDLNA), y)
DFLAGS += -DINCLUDE_MINIDLNA
endif

ifeq ($(INCLUDE_LITE_MINIDLNA), y)
DFLAGS += -DINCLUDE_LITE_MINIDLNA
endif

ifeq ($(INCLUDE_USB_SAMBA_SERVER), y)
DFLAGS += -DINCLUDE_USB_SAMBA_SERVER
endif

ifeq ($(INCLUDE_USB_FTP_SERVER), y)
DFLAGS += -DINCLUDE_USB_FTP_SERVER
endif

ifeq ($(INCLUDE_USB_3G_DONGLE), y)
DFLAGS += -DINCLUDE_USB_3G_DONGLE
endif

ifeq ($(INCLUDE_WAN_MODE), y)
DFLAGS += -DINCLUDE_WAN_MODE
endif

ifeq ($(INCLUDE_QOS), y)
DFLAGS += -DINCLUDE_QOS
endif

ifeq ($(INCLUDE_E8_APP), y)
DFLAGS += -DINCLUDE_E8_APP
endif

ifeq ($(INCLUDE_ANNEXB), y)
DFLAGS += -DINCLUDE_ANNEXB
endif

ifeq ($(INCLUDE_ROUTE_BINDING), y)
DFLAGS += -DINCLUDE_ROUTE_BINDING
endif

ifeq ($(INCLUDE_WAN_VLAN), y)
DFLAGS += -DINCLUDE_WAN_VLAN
endif

ifeq ($(INCLUDE_POLICY_ROUTE), y)
DFLAGS += -DINCLUDE_POLICY_ROUTE
endif

ifeq ($(INCLUDE_L2TP), y)
DFLAGS += -DINCLUDE_L2TP
endif

ifeq ($(INCLUDE_PPTP), y)
DFLAGS += -DINCLUDE_PPTP
endif

ifeq ($(INCLUDE_DUAL_ACCESS), y)
DFLAGS += -DINCLUDE_DUAL_ACCESS
endif

ifeq ($(INCLUDE_PPP_HOST_UNIQ), y)
DFLAGS += -DINCLUDE_PPP_HOST_UNIQ
endif

ifeq ($(INCLUDE_ACL), y)
DFLAGS += -DINCLUDE_ACL
ifeq ($(INCLUDE_ACL_ADVANCE), y)
DFLAGS += -DINCLUDE_ACL_ADVANCE
endif
ifeq ($(INCLUDE_OTHER_ACL), y)
DFLAGS += -DINCLUDE_OTHER_ACL
endif
endif

ifeq ($(INCLUDE_IN_CALEDONIA), y)
DFLAGS += -DINCLUDE_IN_CALEDONIA
endif

ifeq ($(INCLUDE_PORTABLE_APP), y)
DFLAGS += -DINCLUDE_PORTABLE_APP
endif

ifeq ($(INCLUDE_TR143), y)
DFLAGS += -DINCLUDE_TR143
endif

#add by frl,2015-09-14
ifeq ($(INCLUDE_CLOUD), y)
DFLAGS += -DINCLUDE_CLOUD
endif

ifeq ($(INCLUDE_WAN_BLOCK), y)
DFLAGS += -DINCLUDE_WAN_BLOCK
endif

ifeq ($(INCLUDE_WAN_BLOCK_WAN_ERROR), y)
DFLAGS += -DINCLUDE_WAN_BLOCK_WAN_ERROR
endif

ifeq ($(INCLUDE_WAN_BLOCK_FW_UP_INFO), y)
DFLAGS += -DINCLUDE_WAN_BLOCK_FW_UP_INFO
endif
#add end

ifeq ($(INCLUDE_HOSTNAME_EDITABLE), y)
DFLAGS += -DINCLUDE_HOSTNAME_EDITABLE
endif


ifeq ($(INCLUDE_REBOOT_SCHEDULE), y)
DFLAGS += -DINCLUDE_REBOOT_SCHEDULE
endif

ifeq ($(INCLUDE_LED_SCHEDULE), y)
DFLAGS += -DINCLUDE_LED_SCHEDULE
endif

ifeq ($(INCLUDE_VPN), y)
DFLAGS += -DINCLUDE_VPN
endif

ifeq ($(INCLUDE_IPSEC), y)
DFLAGS += -DINCLUDE_IPSEC
endif

ifeq ($(INCLUDE_OPENVPN_SERVER), y)
DFLAGS += -DINCLUDE_OPENVPN_SERVER
endif

ifeq ($(INCLUDE_PPTPVPN_SERVER), y)
DFLAGS += -DINCLUDE_PPTPVPN_SERVER
endif

ifeq ($(INCLUDE_GERMANY_SPEC), y)
DFLAGS += -DINCLUDE_GERMANY_SPEC
endif

ifeq ($(INCLUDE_PPP_RECONN), y)
DFLAGS += -DINCLUDE_PPP_RECONN
endif

ifeq ($(INCLUDE_IGMP_ETHPROXY), y)
DFLAGS += -DINCLUDE_IGMP_ETHPROXY
endif

ifeq ($(INCLUDE_MER), y)
DFLAGS += -DINCLUDE_MER
endif

ifeq ($(INCLUDE_ADSLVID_DETECT), y)
DFLAGS += -DINCLUDE_ADSLVID_DETECT
endif

ifeq ($(INCLUDE_HTTP_SSL), y)
DFLAGS += -DINCLUDE_HTTP_SSL
endif

ifeq ($(INCLUDE_SSH_ACCESS), y)
DFLAGS += -DINCLUDE_SSH_ACCESS
endif

ifeq ($(INCLUDE_FREE_MEMORY_BEFORE_SOFTUP), y)
DFLAGS += -DINCLUDE_FREE_MEMORY_BEFORE_SOFTUP
endif

ifeq ($(INCLUDE_MULTIPLE_NAT), y)
DFLAGS += -DINCLUDE_MULTIPLE_NAT
endif

ifeq ($(INCLUDE_ISP_M2), y)
DFLAGS += -DINCLUDE_ISP_M2
endif

ifeq ($(INCLUDE_VMG1312T20A_V1), y)
DFLAGS += -DINCLUDE_VMG1312T20A_V1
endif

ifeq ($(INCLUDE_DUAL_IMAGE_ZYXEL), y)
DFLAGS += -DINCLUDE_DUAL_IMAGE_ZYXEL
endif

ifeq ($(INCLUDE_ITALY_WISP_USER_ACCOUNT), y)
DFLAGS += -DINCLUDE_ITALY_WISP_USER_ACCOUNT
endif

DFLAGS += -DINCLUDE_$(MODEL)

DF_FLAGS += $(DFLAGS)


ifeq ($(PENNYTEL), y) 
DF_FLAGS += -DPENNYTEL
endif 


WEBFLAGS += WEB_INCLUDE_TEST=0

ifeq ($(INCLUDE_ETHERNET_MULTI_VLAN), y)
WEBFLAGS += INCLUDE_ETHERNET_MULTI_VLAN=1
else
WEBFLAGS += INCLUDE_ETHERNET_MULTI_VLAN=0
endif
ifeq ($(INCLUDE_LOGIN_GDPR_ENCRYPT),y)
WEBFLAGS += INCLUDE_LOGIN_GDPR_ENCRYPT=1
DFLAGS += -DINCLUDE_LOGIN_GDPR_ENCRYPT
else
WEBFLAGS += INCLUDE_LOGIN_GDPR_ENCRYPT=0
endif
ifeq ($(WEB_INCLUDE_DST), y)
WEBFLAGS += WEB_INCLUDE_DST=1
else
WEBFLAGS += WEB_INCLUDE_DST=0
endif

ifeq ($(WEB_INCLUDE_HELP), y)
WEBFLAGS += WEB_INCLUDE_HELP=1
else
WEBFLAGS += WEB_INCLUDE_HELP=0
endif

ifeq ($(INCLUDE_OPTION66), y)
WEBFLAGS += INCLUDE_OPTION66=1
else
WEBFLAGS += INCLUDE_OPTION66=0
endif

ifeq ($(WEB_INCLUDE_MOBILE_UI), y)
WEBFLAGS += WEB_INCLUDE_MOBILE_UI=1
DFLAGS += -DWEB_INCLUDE_MOBILE_UI
else
WEBFLAGS += WEB_INCLUDE_MOBILE_UI=0
endif

ifeq ($(INCLUDE_MULTI_LANGUAGE), y)
WEBFLAGS += INCLUDE_MULTI_LANGUAGE=1
DFLAGS += -DINCLUDE_MULTI_LANGUAGE
else
WEBFLAGS += INCLUDE_MULTI_LANGUAGE=0
endif

ifeq ($(INCLUDE_LANGUAGE_EN_US), y)
WEBFLAGS += INCLUDE_LANGUAGE_EN_US=1
else
WEBFLAGS += INCLUDE_LANGUAGE_EN_US=0
endif

ifeq ($(INCLUDE_LANGUAGE_DE_DE), y)
WEBFLAGS += INCLUDE_LANGUAGE_DE_DE=1
else
WEBFLAGS += INCLUDE_LANGUAGE_DE_DE=0
endif

ifeq ($(INCLUDE_LANGUAGE_IT_IT), y)
WEBFLAGS += INCLUDE_LANGUAGE_IT_IT=1
else
WEBFLAGS += INCLUDE_LANGUAGE_IT_IT=0
endif

ifeq ($(INCLUDE_LANGUAGE_RU_RU), y)
WEBFLAGS += INCLUDE_LANGUAGE_RU_RU=1
else
WEBFLAGS += INCLUDE_LANGUAGE_RU_RU=0
endif

ifeq ($(INCLUDE_LANGUAGE_TR_TR), y)
WEBFLAGS += INCLUDE_LANGUAGE_TR_TR=1
else
WEBFLAGS += INCLUDE_LANGUAGE_TR_TR=0
endif

ifeq ($(INCLUDE_LANGUAGE_PL_PL), y)
WEBFLAGS += INCLUDE_LANGUAGE_PL_PL=1
else
WEBFLAGS += INCLUDE_LANGUAGE_PL_PL=0
endif

ifeq ($(INCLUDE_LANGUAGE_FR_FR), y)
WEBFLAGS += INCLUDE_LANGUAGE_FR_FR=1
else
WEBFLAGS += INCLUDE_LANGUAGE_FR_FR=0
endif

ifeq ($(INCLUDE_LANGUAGE_ES_ES), y)
WEBFLAGS += INCLUDE_LANGUAGE_ES_ES=1
else
WEBFLAGS += INCLUDE_LANGUAGE_ES_ES=0
endif

ifeq ($(INCLUDE_LANGUAGE_ES_LA), y)
WEBFLAGS += INCLUDE_LANGUAGE_ES_LA=1
else
WEBFLAGS += INCLUDE_LANGUAGE_ES_LA=0
endif

ifeq ($(INCLUDE_LANGUAGE_PT_PT), y)
WEBFLAGS += INCLUDE_LANGUAGE_PT_PT=1
else
WEBFLAGS += INCLUDE_LANGUAGE_PT_PT=0
endif

ifeq ($(INCLUDE_LANGUAGE_PT_BR), y)
WEBFLAGS += INCLUDE_LANGUAGE_PT_BR=1
else
WEBFLAGS += INCLUDE_LANGUAGE_PT_BR=0
endif

ifeq ($(INCLUDE_LANGUAGE_EL_GR), y)
WEBFLAGS += INCLUDE_LANGUAGE_EL_GR=1
else
WEBFLAGS += INCLUDE_LANGUAGE_EL_GR=0
endif

ifeq ($(INCLUDE_LANGUAGE_TH_TH), y)
WEBFLAGS += INCLUDE_LANGUAGE_TH_TH=1
else
WEBFLAGS += INCLUDE_LANGUAGE_TH_TH=0
endif

ifeq ($(INCLUDE_WEB_REGION), y)
WEBFLAGS += INCLUDE_WEB_REGION=1
else
WEBFLAGS += INCLUDE_WEB_REGION=0
endif

ifeq ($(INCLUDE_DAY_MONTH_YEAR), y)
WEBFLAGS += INCLUDE_DAY_MONTH_YEAR=1
else
WEBFLAGS += INCLUDE_DAY_MONTH_YEAR=0
endif

ifeq ($(INCLUDE_FLASH_VOICEMAIL), y)
WEBFLAGS += INCLUDE_FLASH_VOICEMAIL=1
DFLAGS += -DINCLUDE_FLASH_VOICEMAIL
else
WEBFLAGS += INCLUDE_FLASH_VOICEMAIL=0
endif

ifeq ($(INCLUDE_DIALPLAN), y)
WEBFLAGS += INCLUDE_DIALPLAN=1
DFLAGS += -DINCLUDE_DIALPLAN
else
WEBFLAGS += INCLUDE_DIALPLAN=0
endif

ifeq ($(INCLUDE_DSL_EWAN_COEXIST), y)
WEBFLAGS += INCLUDE_DSL_EWAN_COEXIST=1
DFLAGS += -DINCLUDE_DSL_EWAN_COEXIST
else
WEBFLAGS += INCLUDE_DSL_EWAN_COEXIST=0
endif

ifeq ($(INCLUDE_ITALY_WISP_USER_ACCOUNT), y)
WEBFLAGS += INCLUDE_ITALY_WISP_USER_ACCOUNT=1
else
WEBFLAGS += INCLUDE_ITALY_WISP_USER_ACCOUNT=0
endif

ifeq ($(INCLUDE_PORT_MIRRORING), y)
DFLAGS += -DINCLUDE_PORT_MIRRORING
WEBFLAGS += INCLUDE_PORT_MIRRORING=1
else
WEBFLAGS += INCLUDE_PORT_MIRRORING=0
endif

ifeq ($(INCLUDE_SESSION_TIMEOUT), y)
DFLAGS += -DINCLUDE_SESSION_TIMEOUT
WEBFLAGS += INCLUDE_SESSION_TIMEOUT=1
else
WEBFLAGS += INCLUDE_SESSION_TIMEOUT=0
endif

ifeq ($(INCLUDE_FULL_REMOTE), y)
DFLAGS += -DINCLUDE_FULL_REMOTE
WEBFLAGS += INCLUDE_FULL_REMOTE=1
else
WEBFLAGS += INCLUDE_FULL_REMOTE=0
endif

ifeq ($(INCLUDE_REMOTE_MULTIPLE_IP), y)
DFLAGS += -DINCLUDE_REMOTE_MULTIPLE_IP
WEBFLAGS += INCLUDE_REMOTE_MULTIPLE_IP=1
else
WEBFLAGS += INCLUDE_REMOTE_MULTIPLE_IP=0
endif

ifeq ($(INCLUDE_AGILE_CONFIG_V2), y)
DFLAGS += -DINCLUDE_AGILE_CONFIG_V2
WEBFLAGS += INCLUDE_AGILE_CONFIG_V2=1
else
WEBFLAGS += INCLUDE_AGILE_CONFIG_V2=0
endif

ifeq ($(INCLUDE_SIMPLE_DIAGNOSTICS), y)
DFLAGS += -DINCLUDE_SIMPLE_DIAGNOSTICS
WEBFLAGS += INCLUDE_SIMPLE_DIAGNOSTICS=1
else
WEBFLAGS += INCLUDE_SIMPLE_DIAGNOSTICS=0
endif
####################################################
#
#Export some variable        
#
####################################################	

export TOOLPREFIX
export HOST
#For modules build & br2684ctl build
export KERNELPATH
#For install kernel modules
export INSTALL_MOD_PATH
#For code control, placed different target file
export OBJ_DIR
#For function conditional compilation
export DF_FLAGS

export TOP_PATH

export OS_LIB_PATH

export PRIVATE_LIB_BIN_PATH

export PRIVATE_APPS_BIN_PATH

export TP_MODULES_PATH

export INCLUDE_SSL_LIB_NAME

#For complier
export CC
export LD
export AR
export STRIP
export RANLIB

include Makefile.$(SUPPLIER)

