######################################################################################
#This Makefile is the main entrance for MediaTek CPE SDK maintaining,including:
#firmware building, SDK release, customer SDK release and other test functions for SDK.
#######################################################################################
#######################################################################################
#In 3 cases you need to pay attention to this makefile:
#(1).You have added or modified private source code created by MediaTeK which can not be released to customers directly but can be provided in object format.
#(2).You will be responsbile for new version SDK release, which means you need to update or upgrade this makefile.
#(3).You need to add special target for SDK maintaining or etc.
########################################################################################

#Entrance for firmware building, which will provide default target if no arguments specified.
ifeq ($(strip $(RELEASEBSP) $(CUSTOMERRELEASE)),)
include Project/MakeFile_Main

else

#Entrance for SDK release (by MediaTek).
ifneq ($(strip $(RELEASEBSP)),)	
ifeq ($(strip $(CUSTOMERRELEASE)),)
release:confirm_infor release_all_check release_backup release_profile release_makefile release_config release_doc release_swept release_tar
endif
endif

#Entrance for customer SDK release (by customers)
ifeq ($(strip $(RELEASEBSP)),)	
ifneq ($(strip $(CUSTOMERRELEASE)),)
customer_release:confirm_infor customer_release_backup customer_release_makefile customer_release_swept customer_release_tar
endif
endif

ifneq ($(strip $(CUSTOM)),)
ifneq ($(strip $(KERNEL_2_6_36)),)
include Project/profile/$(CUSTOM)/$(PROFILE)/$(PROFILE)_2_6_36.profile
else
include Project/profile/$(CUSTOM)/$(PROFILE)/$(PROFILE).profile
endif
else
#CUSTOM is the target custom that will be released, and the default custom will be RT.
CUSTOM=RT
ifneq ($(strip $(PROFILE)),)
ifneq ($(strip $(KERNEL_2_6_36)),)
include Project/profile/$(CUSTOM)/$(PROFILE)/$(PROFILE)_2_6_36.profile
else
include Project/profile/$(CUSTOM)/$(PROFILE)/$(PROFILE).profile
endif
endif
endif
include Project/dir.mak

endif
#Error arguments
ifneq ($(strip $(RELEASEBSP)),)	
ifneq ($(strip $(CUSTOMERRELEASE)),)
error_conflict:
	echo "Error argument! You can not release with RELEASEBSP and CUSTOMERRELEASE at the same time!"
	exit 1;
endif
endif

#PROFILE_SET are the set of release profile supported in this release version 
ifneq ($(strip $(KERNEL_2_6_36)),)
PROFILE_SET=rt65168_release rt63365_release rt63368_release mt7510_release mt7505_release CT_E8B_mt7510_E2_release CT_E8C_PON_release mt7515_release mt7520s_fttdp_release mt7520_release en7521_release en7512_release 
else
PROFILE_SET=tc3162u_release rt65168_release rt63260_release tc3182_release rt63365_release rt63368_release
endif

OPEN_CODE=../open_code
OPEN_CODE_TMP=../open_code_tmp
#UNOPEN_FILE_DELETE means unopen files needed to be deleted
UNOPEN_FILE_DELETE=$(MODULES_PRIV_SRC_DIR)/auto_bench \
$(MODULES_PRIV_SRC_DIR)/en7512_pcie_verify \
$(MODULES_PRIV_SRC_DIR)/sif \
$(MODULES_PRIV_SRC_DIR)/hwnat \
$(MODULES_PRIV_SRC_DIR)/HWNAT_Emulation  \
$(MODULES_PRIV_SRC_DIR)/HWNAT_Emulation_Accelerated  \
$(MODULES_PRIV_SRC_DIR)/net  \
$(APP_AUTOMOUNT_DIR)\
$(APP_PRIVATE_DIR)/auto_mount_dongle \
$(MODULES_CRYPTO_DRIVER) \
$(MODULES_EIP93_DRIVERS) \
$(APP_EIP93_APPS) \
$(FILESYSTEM_DIR)/userfs/SLAD_TestTool \
$(FILESYSTEM_DIR)/userfs/Regression \
$(FILESYSTEM_DIR)/lib/modules/vdriver_93_k.ko \
$(APP_RESTORE_LINOS_INFO_DIR) \
$(APP_SNMPD_DIR) \
$(APP_ZEBRA_DIR) \
$(APP_TCAPI_DIR) \
$(APP_TCWDOG_DIR) \
$(MODULES_HWNAT_DIR) \
$(MODULES_RA_HWNAT_DIR) \
$(MODULES_KPROFILE_DIR) \
$(KERNEL_DIR)/drivers/usb/host/mtk_test \
$(APP_MTK_UH_TEST_DIR) \
$(APP_RA_HWNAT_DIR) \
$(APP_SMUXCTL_DIR) \
$(APP_CENTRAL_COORDINATOR_DIR) \
$(APP_CFG_PARSER_DIR) \
$(APP_CODE_REDUCE_PATCH_DIR) \
$(APP_FWUPGRADE_DIR) \
$(APP_HUAWEI_CI_DIR) \
$(APP_PRIVATE_DIR)/igmpv3proxy \
$(APP_SNMPD_DIR) \
$(APP_PCM_TEST_DIR) \
$(APP_QUAGGA_DIR) \
$(APP_RCS_DIR) \
$(APP_EPON_DUMMY_DETECT_DIR) \
$(APP_SLIC_TEST_DIR) \
$(APP_SKB_MANAGER_DIR) \
$(APP_PWCTLCMD_DIR) \
$(APP_DMS_DIR) \
$(APP_RA_MENU_DIR) \
$(APP_AUTOIP_DIR) \
$(APP_PRIVATE_DIR)/osgi \
$(OSGI_DIR)/antlr \
$(OSGI_DIR)/src \
$(OSGI_DIR)/java \
$(OSGI_DIR)/classpath-new \
$(OSGI_DIR)/jamvm-2.0.0 \
$(OSGI_DIR)/zlib-1.2.8 \
$(APP_JOYME_DIR)/example \
$(APP_JOYME_DIR)/joyme.c \
$(APP_CTSGWLIB_DIR)/ctsgw.c \
$(APP_CTSGWLIB_DIR)/ctsgw_test \
$(APP_BUNDLELIB_DIR)/bundle.c \
$(APP_BANDWIDTH_DIR) \
$(APP_PLUGIN_MONITOR_DIR) \
$(APP_STORE_DIR) \
$(APP_HTTPDETECT_DIR) \
$(BUNDLE_MONITOR_DIR) \
$(BUNDLE_CMD_DIR) \
$(USB_MONITOR_DIR) \
$(TRUNK_DIR)/buildfs_vs_fw_table.xls \
$(TRUNK_DIR)/rootfs \
$(TRUNK_DIR)/README \
$(TRUNK_DIR)/bootrom_int \
$(TRUNK_DIR)/modules_tmp/ko/modules/2.6.22.15 \
$(MODULES_PRIV_SRC_DIR)/tc3162l2hp2h/SAR_VERIFY \
$(MODULES_PRIV_SRC_DIR)/tc3162l2hp2h/*_verify.c \
$(MODULES_PRIV_SRC_DIR)/tc3162l2hp2h/tc3162_udc20.c \
$(MODULES_PRIV_SRC_DIR)/tc3162l2hp2h/tc3162_udc.c \
$(MODULES_PRIV_SRC_DIR)/tcci/version.c \
$(APP_PRIVATE_DIR)/flash \
$(APP_PRIVATE_DIR)/tcapi_enhance_lib \
$(APP_PRIVATE_DIR)/sendicmp \
$(MODULES_PRIV_SRC_DIR)/voip \
$(MODULES_PRIV_SRC_DIR)/fe_ppe_test \
$(MODULES_PRIV_SRC_DIR)/slm_verify \
$(APP_TCAPILIB_DIR)/*.c 
ifneq ($(strip $(TCSUPPORT_CPU_EN7512) $(TCSUPPORT_CPU_EN7521)),)
UNOPEN_FILE_DELETE += $(MODULES_PRIV_SRC_DIR)/raeth \
$(MODULES_QDMA_DIR)
endif
ifeq ($(strip $(TCSUPPORT_BONDING)),)	
UNOPEN_FILE_DELETE += $(MODULES_PRIV_SRC_DIR)/bonding_pcie
endif
ifeq ($(strip $(TCSUPPORT_XPON_HAL_API_MCST)),)
UNOPEN_FILE_DELETE += $(MODULES_PRIV_SRC_DIR)/mtk_multicast
endif
ifeq ($(strip $(TCSUPPORT_MUH_TEST)),)
UNOPEN_FILE_DELETE += $(APP_PRIVATE_DIR)/mtk_xhci_test_cli
endif
ifeq ($(strip $(TCSUPPORT_XPON_HAL_API_EXT)),)
UNOPEN_FILE_DELETE += $(APP_PRIVATE_DIR)/mtk_itf
endif
ifeq ($(strip $(TCSUPPORT_CT_PHONEAPP)),)
UNOPEN_FILE_DELETE += $(APP_PRIVATE_DIR)/mobile-manager \
$(APP_PRIVATE_DIR)/TR69_64/midware-server 
endif
ifeq ($(strip $(TCSUPPORT_CT_PON_GD)),)
UNOPEN_FILE_DELETE += $(APP_CWMP_DIR)/clmp \
$(APP_CWMP_DIR)/cwmp/Sources \
$(APP_CWMP_DIR)/main/main.c \
$(APP_CWMP_DIR)/linos/utily.c
endif
UNOPEN_FILE_DELETE += $(APP_CWMP_DIR)/rompager \
$(APP_CMDCI_DIR)/algcmd.* \
$(APP_CMDCI_DIR)/firewallcmd.* \
$(APP_CMDCI_DIR)/portbindcmd.* \
$(APP_CMDCI_DIR)/qoscmd.* \
$(APP_CMDCI_DIR)/qoscmd_ct.* \
$(APP_CMDCI_DIR)/accesslimitcmd \
$(APP_CMDCI_DIR)/accesslimitcmd.* \
$(APP_CMDCI_DIR)/atmcmdd \
$(APP_CMDCI_DIR)/atmcmdc.* \
$(APP_FONSMCD_DIR)/src/coova \
$(APP_FONSMCD_DIR)/src/fonsmcd \
$(APP_FONSMCD_DIR)/src/fon-api \
$(APP_FONSMCD_DIR)/src/getrandom/getrandom.o \
$(APP_FONSMCD_DIR)/src/getrandom/getrandom \
$(FILESYSTEM_DIR)/boaroot/cgi-bin/WizardOi \
$(FILESYSTEM_DIR)/boaroot/html/include	\
$(BOOTROM_DIR)/ddr_cal/*.c \
$(BOOTROM_DIR)/ddr_cal_mt7505/*.c \
$(BOOTROM_DIR)/ddr_cal_en7512/*.c \
$(APP_NEW_SHELL_DIR) \
$(APP_FON_HOTSPOT_DIR) \
$(APP_SIM_CARD_DIR) \
$(APP_BLOCK_PROCESS_DIR)

ifeq ($(strip $(RELEASEHWNAT)),)
UNOPEN_FILE_DELETE += $(MODULES_RA_HWNAT_7510_DIR) \
$(APP_RA_HWNAT_7510_DIR) 
endif

#delete adsl or vdsl code for mt7520
UNOPEN_FILE_DELETE += $(MODULES_PRIV_SRC_DIR)/epon
ifneq ($(strip $(TCSUPPORT_CPU_MT7520)),)
UNOPEN_FILE_DELETE += $(MODULES_PRIV_SRC_DIR)/dmt \
$(MODULES_PRIV_SRC_DIR)/mt7510_ptm \
$(MODULES_PRIV_SRC_DIR)/mtk_sar \
$(MODULES_PRIV_SRC_DIR)/tc3262
endif

#delete xpon_lib
UNOPEN_FILE_DELETE += $(LIB_XPON_DIR)/epon/libepon.c \
$(LIB_XPON_DIR)/eponmap/libeponmap.c \
$(LIB_XPON_DIR)/gponmap/libgponmap.c \
$(LIB_XPON_DIR)/pon_vlan/lib_pon_vlan.c \
$(LIB_XPON_DIR)/pon_mac_filter/lib_pon_mac_filter.c \
$(LIB_XPON_DIR)/xpon_igmp/lib_xpon_igmp.c \
$(LIB_XPON_DIR)/xpon_igmp/xpon_igmp_omci.c

#delete oam 
UNOPEN_FILE_DELETE += $(APP_EPON_OAM_DIR)/epon_oam.c \
$(APP_EPON_OAM_DIR)/epon_oam_ext.c \
$(APP_EPON_OAM_DIR)/epon_oam_dbg_srv.c \
$(APP_EPON_OAM_DIR)/epon_oam_timer.c \
$(APP_EPON_OAM_DIR)/epon_oam_util.c

#delete mt75xx sim 
UNOPEN_FILE_DELETE += $(MODULES_PRIV_SRC_DIR)/mtk_sim 
ifeq ($(strip $(TCSUPPORT_CT_SIMCARD_SEPARATION)),)
UNOPEN_FILE_DELETE += $(MODULES_PRIV_SRC_DIR)/simcard_separation 
endif
ifeq ($(strip $(TCSUPPORT_SIMCARD_GENERAL)),)
UNOPEN_FILE_DELETE += $(MODULES_PRIV_SRC_DIR)/simcard_separation_general
endif

#voip xpon for MT7505
ifeq ($(strip $(PROFILE)),mt7505_release)
UNOPEN_FILE_DELETE += $(MODULES_XPON_WAN_DIR) \
$(MODULES_PRIV_SRC_DIR)/voip_2.6.36 \
$(MODULES_PRIV_SRC_DIR)/voip \
$(APP_PRIVATE_DIR)/voip \
$(APP_XPON_DIR) \
$(APP_FLASH_DIR) \
$(APP_ECB3LIB_DIR) \
$(TRUNK_DIR)/windows_rndis_driver
endif

#delete omci 
UNOPEN_FILE_DELETE += $(APP_OMCI_DIR)/omci.c \
$(APP_OMCI_DIR)/omci_dbg_srv.c \
$(APP_OMCI_DIR)/omci_general_func.c \
$(APP_OMCI_DIR)/omci_message_handle.c \
$(APP_OMCI_DIR)/omci_timer.c \
$(APP_OMCI_DIR)/omci_utily.c \

#delete linux kernel dir for this release don't support the other kernel to compile
ifneq ($(strip $(KERNEL_2_6_36)),)
UNOPEN_FILE_DELETE += $(TRUNK_DIR)/linux
else
UNOPEN_FILE_DELETE += $(TRUNK_DIR)/linux-2.6.36
endif

#delete midware & upnp-dm
UNOPEN_FILE_DELETE += $(APP_PRIVATE_DIR)/TR69_64/cwmp/midware \
$(APP_UPNP_DM_DIR)

#delete extra code of CT if customer is not CT and do the other when CT defined 
ifeq ($(strip $(TCSUPPORT_CT)),)
UNOPEN_FILE_DELETE += $(APP_PRIVATE_DIR)/cfg_manager_ct \
$(APP_PRIVATE_DIR)/cfg_manager/cfg_manager.c \
$(APP_PRIVATE_DIR)/cfg_manager/*.o \
$(APP_PRIVATE_DIR)/ethcmd_ct \
$(APP_PRIVATE_DIR)/TR69_64/cwmp/cwmpParameter_ct \
$(APP_CMDCI_DIR)/atmcmdd.* \
$(APP_PRIVATE_DIR)/backuprestore 
else
UNOPEN_FILE_DELETE += $(APP_PRIVATE_DIR)/cfg_manager \
$(APP_PRIVATE_DIR)/cfg_manager_ct/cfg_manager.c \
$(APP_PRIVATE_DIR)/cfg_manager_ct/*.o \
$(APP_PRIVATE_DIR)/ethcmd_ct \
$(APP_CMDCI_DIR)/atmcmdd.c \
$(APP_PRIVATE_DIR)/TR69_64/cwmp/cwmpParameter \
$(APP_PRIVATE_DIR)/backuprestore \
$(APP_ETC_SCRIPT_DIR)/ct/CDTool \
$(APP_ETC_SCRIPT_DIR)/ct/eHomeEIWrd_e8bc_20130609.zip \
$(APP_CWMP_DIR)/cwmp/cwmpParameter_ct/Sources/cp_X_CTCOM_ProxyDevice_func.c \
$(APP_CWMP_DIR)/cwmp/cwmpParameter_ct/Sources/cp_X_CTCOM_ProxyDevice_nodetable.c 
endif

UNOPEN_FILE_DELETE += $(MOBILE_MANAGEER_DIR)/app-client \
$(MOBILE_MANAGEER_DIR)/app-server \
$(MOBILE_MANAGEER_DIR)/common \
$(MOBILE_MANAGEER_DIR)/exec-process \
$(MOBILE_MANAGEER_DIR)/json-1.5 \
$(MOBILE_MANAGEER_DIR)/plugin_server \
$(MOBILE_MANAGEER_DIR)/src/* \
$(TOOLS_DIR)/yaffs2-HEAD-583dbd9 \
$(APP_STB_TEST_DIR)

ifneq ($(strip $(TCSUPPORT_CT_PON)),)
UNOPEN_FILE_DELETE += $(APP_TCAPILIB_ENHANCE_DIR) \
$(APP_TCAPI_CWMP_LIB_DIR) \
$(APP_SENDICMP_DIR)
endif

ifeq ($(strip $(TCSUPPORT_CT_JOYME)),)
UNOPEN_FILE_DELETE += $(APP_JOYME_DIR) \
$(APP_BUNDLELIB_DIR) \
$(APP_CTSGWLIB_DIR) \
$(OSGI_DIR)
endif

ifeq ($(strip $(TCSUPPORT_CPU_MT7505)),)
UNOPEN_FILE_DELETE += $(BOOTROM_DIR)/ddr_cal_mt7505 \
$(BOOTROM_DIR)/start.S_7505_final
endif

#FTTDP
UNOPEN_FILE_DELETE += $(MODULES_FTTDP_INIC_DIR) \
$(APP_TCAPI_INIC_DIR) \
$(APP_TCAPILIB_INIC_DIR) \
$(APP_INIC_CLIENT_HEARTBEAT_DIR)

OTHER_FILE_DELETE= \
$(PROJECT_DIR)/images/tcboot.bin \
$(PROJECT_DIR)/images/tclinux.bin \
$(PROJECT_DIR)/images/tclinux_allinone \
$(PROJECT_DIR)/images/tclinux_allinone.swap \
$(FILESYSTEM_DIR)/dev/* \
$(TRUNK_DIR)/backup \
$(MODULES_PRIV_SRC_DIR)/wifi/ATHR-GEMINI-AP9.0.0.33.tgz \
$(MODULES_PRIV_SRC_DIR)/wifi/2008_0506_RT61_Linux_SoftAP_Drv1.1.3.0 \
$(MODULES_PRIV_SRC_DIR)/wifi/2009_0209_RT3090_LinuxAP_V2.1.3.0 \
$(MODULES_PRIV_SRC_DIR)/wifi/2009_0904_RT3090_LinuxAP_v2.3.0.0_TC_for_3092 \

#Backup_Folder are the backup location for release requisite
BACKUP_FOLDERS=$(APP_BINARY_DIR) \
$(MODULES_PRIV_SRC_DIR)/ko \
$(APP_TCAPILIB_DIR) \
$(APP_PRIVATE_DIR)/TR69_64 \

ifeq ($(strip $(TCSUPPORT_CT)),)
BACKUP_FOLDERS += $(APP_PRIVATE_DIR)/cfg_manager
else
BACKUP_FOLDERS += $(APP_PRIVATE_DIR)/cfg_manager_ct
endif

INFOR0="Attention!!!!"
ifneq ($(strip $(KERNEL_2_6_36)),)
INFOR1="This command can only release SDK with 2.6.36.0 kernel"
INFOR2="If you want to release 2.6.22.15 kernel, please use cmd: make RELEASEBSP=y"
else
INFOR1="This command can only release SDK with 2.6.22.15 kernel"
INFOR2="If you want to release 2.6.36 kernel, please use cmd: make RELEASEBSP=y KERNEL_2_6_36=y"
endif
INFOR3="This command will release Multi-chip SDK, which will remove the un-open files and can't be recovered!!!!!"
INFOR4="Please select the release profile desired. Example: 1 2 3"
INFOR5="Please input your selection:"
INFOR6="Profile you selected is as following:"
INFOR7="if you agree, please input "y",else if you wanna break please input "N". [y/N] ?"
EXIT_INFOR="Your input is not y! The make process will be breaked!"
CONTINUE_INFOR="Entering make process......"
INFOR_CUSTOM="If you wanan release a customer version from SDK, please make sure your command is: "make -f MultiChip.make CUSTOM_RELASE=y""
SEPARATOR="....................................................................................................."


#if RD wanna release with specified profile, the select and confirm will be canceld
ifneq ($(strip $(PROFILE)),)
PROFILE_SELECT = $(PROFILE)
confirm_infor:
	@echo "release with $(PROFILE) profile now..."
	@sleep 1
else
#if RD wanna release with some dedicated profiles, the select and confirm will be triggerd
confirm_infor:
	@echo "****************************************************************************************************"
	@echo $(INFOR0)
	@echo "....................................................................................................."
	@echo $(INFOR1)
	@echo $(INFOR2)
	@echo "....................................................................................................."
	@echo $(INFOR3)
	@echo $(INFOR4)
	@id=0;\
	for profile in $(PROFILE_SET);\
	do \
		echo [$$id] $$profile;\
		id=`expr $$id + 1`; \
	done;
	@echo "****************************************************************************************************"
	@echo $(INFOR5);
	@rm -rf "./.profile_select.tmp";
	@profile_set=(${PROFILE_SET}); \
	PROFILE_SET_NUM=$${profile_set[@]}; \
	read select;\
	echo $(SEPARATOR);\
	while [ "$$select"x == ""x ] ;\
	do \
		echo $(INFOR5);read select;\
	done; \
	while [ "$$select"x == ""x ] ;\
	do \
		fail=0;\
		for index in $$select; \
		do \
			if [[ ( $$index -lt 0 ) || ( $$index -ge $${PROFILE_SET_NUM} ) ]]; \
			then\
				echo "******Wrong input! Please check again!*****"; \
				fail=1; \
				echo $(INFOR5);read select;\
				while [ "$$select"x == ""x ]];\
				do \
					echo $(INFOR5);read select;\
				done; \
				break; \
			fi; \
		done ;\
		if [ $$fail -eq 1 ];\
		then \
			continue; \
		else \
			break; \
		fi; \
	done;\
	echo $(INFOR6);\
	for index in $$select; \
	do \
		echo "******$${profile_set[$$index]}*****"; \
		echo $${profile_set[$$index]}>>"./.profile_select.tmp"; \
	done
	@echo $(INFOR7);
	@read var ;\
	if [[ $$var == y ]]; \
	then \
		echo $(CONTINUE_INFOR); \
	else \
		echo "" >"./.profile_select.tmp";\
		echo $(EXIT_INFOR);\
		exit 1;\
	fi
	@echo $(SEPARATOR)
	@sleep 1;

PROFILE_SELECT=`cat ./.profile_select.tmp`
endif	

release_all_check:release_checktools
	rm -rf ./.profile_summary
ifneq ($(strip $(KERNEL_2_6_36)),)
	for release in ${PROFILE_SELECT} ; \
	do \
			cat Project/profile/$(CUSTOM)/$${release}/$${release}_2_6_36.profile >>./.profile_summary ;\
	done
else
	for release in ${PROFILE_SELECT} ; \
	do \
		cat Project/profile/$(CUSTOM)/$${release}/$${release}.profile >>./.profile_summary ;\
	done
endif


	if test -e $(TOOLS_DIR)/cplopts_checktool/tmp/restore.sh; \
	then echo "Please restore the files first before any modification!"; exit 1; \
	else echo "executing all check"; \
	fi

ifneq ($(strip $(CHKDIR)),)
	echo "checking CHDIR"
	$(TOOLS_DIR)/cplopts_begin $(TRUNK_DIR)/$(CHKDIR) ./.profile_summary
else
	echo "checking all files in trunk dir"
ifeq ($(CUSTOM),CT)	
	$(TOOLS_DIR)/cplopts_begin $(TRUNK_DIR) ./.profile_summary $(PROJECT_MENUCONFIG_DIR)/$(CUSTOM).config
else
	$(TOOLS_DIR)/cplopts_begin $(TRUNK_DIR) ./.profile_summary
endif
endif
	if test -e $(TOOLS_DIR)/cplopts_checktool/tmp/error; \
	then echo "Compile code error"; exit 1; \
	fi
	if test -e $(TOOLS_DIR)/cplopts_checktool/tmp/parsingfile; \
	then echo "Tool couldn't work totally! The error filename is";\
			 cat $(TOOLS_DIR)/cplopts_checktool/tmp/parsingfile;rm -f $(TOOLS_DIR)/cplopts_checktool/tmp/parsingfile;exit 1; \
	else echo "Tool works totally and Compile code success"; \
	fi

	rm -rf ./.profile_summary 

release_restore:release_checktools
	if test -e $(TOOLS_DIR)/cplopts_checktool/tmp/restore.sh; \
	then echo "Compile bootbase success"; \
	/bin/sh $(TOOLS_DIR)/cplopts_checktool/tmp/restore.sh; \
	rm -f $(TOOLS_DIR)/cplopts_checktool/tmp/restore.sh; \
	rm -rf $(TRUNK_DIR)/backup; \
	fi
	echo "Files  restored!"

release_checktools:
	echo making compile options checking  tools
#       make -C $(TOOLS_DIR)/cplopts_checktool clean
	make -C $(TOOLS_DIR)/cplopts_checktool
	cp -f $(TOOLS_DIR)/cplopts_checktool/cplopts_begin $(TOOLS_DIR)
	cp -f $(TOOLS_DIR)/cplopts_checktool/cplopts_formcheck $(TOOLS_DIR)
	cp -f $(TOOLS_DIR)/cplopts_checktool/cplopts_remove $(TOOLS_DIR)

release_checktools_clean:
	echo cleaning compile options checking  tools
	make -C $(TOOLS_DIR)/cplopts_checktool clean


#this target will do "make" process for SDK release, with loop of clean>>>make profile[i]>>>backup *.ko , *.o>>> clean....
release_backup:
ifneq ($(strip $(TCSUPPORT_CT_CUSTOMCODE)),)
	mkdir -p $(OPEN_CODE) && mkdir -p $(OPEN_CODE_TMP)
	# cfg_manager
	mkdir -p $(OPEN_CODE_TMP)/$(APP_CFG_MANAGER_DIR)
	cp $(APP_CFG_MANAGER_DIR)/* $(OPEN_CODE_TMP)/$(APP_CFG_MANAGER_DIR)
	# cwmp source
	mkdir -p $(OPEN_CODE_TMP)/$(APP_CWMP_DIR)/cwmp/Sources
	cp $(APP_CWMP_DIR)/cwmp/Sources/* $(OPEN_CODE_TMP)/$(APP_CWMP_DIR)/cwmp/Sources
	# cmd-ci
	mkdir -p $(OPEN_CODE_TMP)/$(APP_CMDCI_DIR)
	cp $(APP_CMDCI_DIR)/accesslimitcmd.* $(OPEN_CODE_TMP)/$(APP_CMDCI_DIR)
	cp $(APP_CMDCI_DIR)/firewallcmd.* $(OPEN_CODE_TMP)/$(APP_CMDCI_DIR)
	cp $(APP_CMDCI_DIR)/portbindcmd.* $(OPEN_CODE_TMP)/$(APP_CMDCI_DIR)
	cp $(APP_CMDCI_DIR)/algcmd.* $(OPEN_CODE_TMP)/$(APP_CMDCI_DIR)
	cp $(APP_CMDCI_DIR)/qoscmd.* $(OPEN_CODE_TMP)/$(APP_CMDCI_DIR)
	mkdir -p $(OPEN_CODE_TMP)/$(APP_RA_HWNAT_7510_DIR)
	cp $(APP_RA_HWNAT_7510_DIR)/* $(OPEN_CODE_TMP)/$(APP_RA_HWNAT_7510_DIR)
	# xpon app
	mkdir -p $(OPEN_CODE_TMP)/$(APP_XPON_DIR)
	cp -a $(APP_XPON_DIR)/* $(OPEN_CODE_TMP)/$(APP_XPON_DIR)
	# modules
	mkdir -p $(OPEN_CODE_TMP)/$(MODULES_ACCESSLIMIT_DIR)
	cp $(MODULES_ACCESSLIMIT_DIR)/* $(OPEN_CODE_TMP)/$(MODULES_ACCESSLIMIT_DIR)
	mkdir -p $(OPEN_CODE_TMP)/$(MODULES_PON_VLAN_DIR)
	cp $(MODULES_PON_VLAN_DIR)/* $(OPEN_CODE_TMP)/$(MODULES_PON_VLAN_DIR)
	mkdir -p $(OPEN_CODE_TMP)/$(MODULES_URL_FILTER_DIR)
	cp $(MODULES_URL_FILTER_DIR)/* $(OPEN_CODE_TMP)/$(MODULES_URL_FILTER_DIR)
	mkdir -p $(OPEN_CODE_TMP)/$(MODULES_RA_HWNAT_7510_DIR)
	cp $(MODULES_RA_HWNAT_7510_DIR)/* $(OPEN_CODE_TMP)/$(MODULES_RA_HWNAT_7510_DIR)
	mkdir -p $(OPEN_CODE_TMP)/$(MODULES_RAETH_DIR)
	cp $(MODULES_RAETH_DIR)/* $(OPEN_CODE_TMP)/$(MODULES_RAETH_DIR)
	mkdir -p $(OPEN_CODE_TMP)/$(MODULES_CT_VLAN_TAG_DIR)
	cp $(MODULES_CT_VLAN_TAG_DIR)/* $(OPEN_CODE_TMP)/$(MODULES_CT_VLAN_TAG_DIR)

	mkdir -p $(OPEN_CODE_TMP)/$(MODULES_IGMPSNOOP_DIR)
	cp $(MODULES_IGMPSNOOP_DIR)/* $(OPEN_CODE_TMP)/$(MODULES_IGMPSNOOP_DIR)
	mkdir -p $(OPEN_CODE_TMP)/$(MODULES_SWQOS_DIR)
	cp $(MODULES_SWQOS_DIR)/* $(OPEN_CODE_TMP)/$(MODULES_SWQOS_DIR)
	mkdir -p $(OPEN_CODE_TMP)/$(MODULES_XPON_WAN_DIR)
	cp -a $(MODULES_XPON_WAN_DIR)/* $(OPEN_CODE_TMP)/$(MODULES_XPON_WAN_DIR)
	mkdir -p $(OPEN_CODE_TMP)/$(MODULES_LOOP_DETECT_DIR)
	cp $(MODULES_LOOP_DETECT_DIR)/* $(OPEN_CODE_TMP)/$(MODULES_LOOP_DETECT_DIR)
	mkdir -p $(OPEN_CODE_TMP)/$(MODULES_PRIV_SRC_DIR)/tcci
	cp $(MODULES_PRIV_SRC_DIR)/tcci/* $(OPEN_CODE_TMP)/$(MODULES_PRIV_SRC_DIR)/tcci
	mkdir -p $(OPEN_CODE_TMP)/$(MODULES_XPONIGMP_DIR)
	cp $(MODULES_XPONIGMP_DIR)/* $(OPEN_CODE_TMP)/$(MODULES_XPONIGMP_DIR)

	mkdir -p $(OPEN_CODE_TMP)/$(MODULES_PON_MAC_FILTER_DIR)
	cp $(MODULES_PON_MAC_FILTER_DIR)/* $(OPEN_CODE_TMP)/$(MODULES_PON_MAC_FILTER_DIR)	
	mkdir -p $(OPEN_CODE_TMP)/$(MODULES_PORTBIND_DIR)
	cp $(MODULES_PORTBIND_DIR)/* $(OPEN_CODE_TMP)/$(MODULES_PORTBIND_DIR)	
	mkdir -p $(OPEN_CODE_TMP)/$(MODULES_XPON_MAP_DIR)
	cp $(MODULES_XPON_MAP_DIR)/* $(OPEN_CODE_TMP)/$(MODULES_XPON_MAP_DIR)		
	mkdir -p $(OPEN_CODE_TMP)/$(MODULES_FE_7512_DIR)
	cp $(MODULES_FE_7512_DIR)/* $(OPEN_CODE_TMP)/$(MODULES_FE_7512_DIR)

	mkdir -p $(OPEN_CODE_TMP)/$(MODULES_QDMA_7512_DIR)
	cp $(MODULES_QDMA_7512_DIR)/* $(OPEN_CODE_TMP)/$(MODULES_QDMA_7512_DIR)

	mkdir -p $(OPEN_CODE_TMP)/$(MODULES_ETHER_DIR)
	cp -a $(MODULES_ETHER_DIR)/* $(OPEN_CODE_TMP)/$(MODULES_ETHER_DIR)
	

	cp -a $(OPEN_CODE_TMP)/$(TRUNK_DIR)/* $(OPEN_CODE)/
	tar -czvf ../sdk_opencode_`date +%Y%m%d`.tgz $(OPEN_CODE)/*
	rm -rf $(OPEN_CODE_TMP) $(OPEN_CODE)
endif
	cp -rf $(TRUNK_DIR)/filesystem $(TRUNK_DIR)/filesystem_clean;
ifneq ($(strip $(KERNEL_2_6_36)),)
	for release in ${PROFILE_SELECT} ; \
	do \
		echo "*************************************************"; \
		echo "*************************************************"; \
		echo "*****$$release making and backup in progress********"; \
		echo "*************************************************"; \
		sleep 3;\
		echo "*************************************************"; \
		${MAKE} -f "$(PROJECT_DIR)/MakeFile_Main" PROFILE=$${release} KERNEL_2_6_36=1 CUSTOM=${CUSTOM} clean ; if [ $$? != 0 ];then exit 1;fi; \
		echo "*************************************************"; \
		${MAKE} -f "$(PROJECT_DIR)/MakeFile_Main"  PROFILE=$${release} KERNEL_2_6_36=1 CUSTOM=${CUSTOM} RELEASEBSP=y bootbase kernel modules apps; if [ $$? != 0 ];then exit 1;fi; \
		echo "*************************************************"; \
		${MAKE} -f "$(PROJECT_DIR)/MakeFile_Main" PROFILE=$${release} KERNEL_2_6_36=1 CUSTOM=${CUSTOM} RELEASEBSP=y release_end;if [ $$? != 0 ];then exit 1;fi; \
		echo "**************************************************"; \
		${MAKE} -f "$(PROJECT_DIR)/MakeFile_Main" PROFILE=$${release} KERNEL_2_6_36=1 CUSTOM=${CUSTOM} clean ; if [ $$? != 0 ];then exit 1;fi; \
		echo "**************************************************"; \
	done
else
	for release in ${PROFILE_SELECT} ; \
	do \
		echo "*************************************************"; \
		echo "*************************************************"; \
		echo "*****$$release making and backup in progress********"; \
		echo "*************************************************"; \
		sleep 3;\
		echo "*************************************************"; \
		${MAKE} -f "$(PROJECT_DIR)/MakeFile_Main" PROFILE=$${release} CUSTOM=${CUSTOM} clean ; if [ $$? != 0 ];then exit 1;fi; \
		echo "*************************************************"; \
		${MAKE} -f "$(PROJECT_DIR)/MakeFile_Main"  PROFILE=$${release} CUSTOM=${CUSTOM} RELEASEBSP=y kernel modules apps; if [ $$? != 0 ];then exit 1;fi; \
		echo "*************************************************"; \
		${MAKE} -f "$(PROJECT_DIR)/MakeFile_Main" PROFILE=$${release} CUSTOM=${CUSTOM} RELEASEBSP=y release_end;if [ $$? != 0 ];then exit 1;fi; \
		echo "**************************************************"; \
		${MAKE} -f "$(PROJECT_DIR)/MakeFile_Main" PROFILE=$${release} CUSTOM=${CUSTOM} clean ; if [ $$? != 0 ];then exit 1;fi; \
		echo "**************************************************"; \
	done
endif

	rm -rf $(TRUNK_DIR)/filesystem;
	mv -f  $(TRUNK_DIR)/filesystem_clean $(TRUNK_DIR)/filesystem

#Make sure: (1)Profile locates in CUSTOM has been removed and (2) Profile that not named with *_demo in CUSTOM foder have been removed
release_profile:
	find $(TRUNK_DIR)/Project/profile/* -maxdepth 0 -type d -not -name "$(CUSTOM)" | xargs rm -rf
	find $(TRUNK_DIR)/Project/profile/$(CUSTOM)/* -maxdepth 0 -not -name '*_demo' | xargs rm -rf
	

#Substitue makefile in special folder. These targets mostly maintain different codes before and after SDK release, and hence need to use makefile.relese instead.
release_makefile:
	#copy makefiles
	cp $(MODULES_PRIV_SRC_DIR)/tc3162l2hp2h/Makefile.release $(MODULES_PRIV_SRC_DIR)/tc3162l2hp2h/Makefile
ifeq ($(strip $(TCSUPPORT_CT)),)
	cp $(APP_PRIVATE_DIR)/cfg_manager/Makefile.release $(APP_PRIVATE_DIR)/cfg_manager/Makefile	
else
	cp $(APP_PRIVATE_DIR)/cfg_manager_ct/Makefile.release $(APP_PRIVATE_DIR)/cfg_manager_ct/Makefile
endif
	cp $(APP_TCAPILIB_DIR)/Makefile.release $(APP_TCAPILIB_DIR)/Makefile

	cp $(APP_CMDCI_DIR)/Makefile.release $(APP_CMDCI_DIR)/Makefile
	cp $(APP_CWMP_DIR)/Makefile.release $(APP_CWMP_DIR)/Makefile
	cp $(BOOTROM_DIR)/ddr_cal/Makefile.release $(BOOTROM_DIR)/ddr_cal/Makefile
	cp $(BOOTROM_DIR)/ddr_cal_mt7505/Makefile.release $(BOOTROM_DIR)/ddr_cal_mt7505/Makefile
	cp $(BOOTROM_DIR)/ddr_cal_en7512/Makefile.release $(BOOTROM_DIR)/ddr_cal_en7512/Makefile
ifneq ($(strip $(TCSUPPORT_CT_JOYME)),)
	cp $(APP_CTSGWLIB_DIR)/Makefile.release $(APP_CTSGWLIB_DIR)/Makefile
	cp $(APP_BUNDLELIB_DIR)/Makefile.release $(APP_BUNDLELIB_DIR)/Makefile
endif
ifneq ($(strip $(TCSUPPORT_CT_PHONEAPP)),)
	cp $(MOBILE_MANAGEER_DIR)/Makefile.release $(MOBILE_MANAGEER_DIR)/Makefile
endif


igmpsnooping:
	cd $(MODULES_IGMPSNOOP_DIR) && make all

#clear up the documents in SDK
release_doc:
	chmod 777 -R $(TRUNK_DIR)/doc
	cp -f $(TRUNK_DIR)/doc/MediaTek_Linux_SDK_Release_Notes.pdf $(TRUNK_DIR)/
	find $(TRUNK_DIR)/doc -mindepth 1 -maxdepth 1 -not -name "*SOP" -not -name "*MANUAL" | xargs rm -rf
	find $(TRUNK_DIR)/doc/SOP -type f -not -name "*.pdf" |xargs rm -rf
	find $(TRUNK_DIR)/doc/MANUAL -type f -not -name "*.pdf" |xargs rm -rf
ifneq ($(strip $(TCSUPPORT_CPU_EN7512) $(TCSUPPORT_CPU_EN7521)),)
	rm -rf $(TRUNK_DIR)/*.pdf
endif	
	
#This target is to add key parameter in Config.in to indicate that the code package has been released. The option "TCSUPPORT_RELEASED_CODE" will be used in controlling of customer configure.
release_config:
	echo "choice" >> Project/config/menuconfig/Config.in
	echo "	  prompt \"Release Type\""  >> Project/config/menuconfig/Config.in
	echo "config TCSUPPORT_RELEASED_CODE" >> Project/config/menuconfig/Config.in
	echo "    bool \"TCSUPPORT_RELEASED_CODE RT\"" >> Project/config/menuconfig/Config.in
	echo "    help"  >> Project/config/menuconfig/Config.in
	echo "         The flag of code release" >> Project/config/menuconfig/Config.in
	echo "endchoice" >> Project/config/menuconfig/Config.in
ifneq ($(strip $(KERNEL_2_6_36)),)
	echo "config TC_RELEASE_2_6_36_KERNEL" >> Project/config/menuconfig/Config.in
	echo "    bool \"TC_RELEASE_2_6_36_KERNEL\"" >> Project/config/menuconfig/Config.in
	echo "		default y" >> Project/config/menuconfig/Config.in
else
	echo "config TC_RELEASE_2_6_22_15_KERNEL" >> Project/config/menuconfig/Config.in
	echo "    bool \"TC_RELEASE_2_6_22_15_KERNEL\"" >> Project/config/menuconfig/Config.in
	echo "		default y" >> Project/config/menuconfig/Config.in
endif
ifneq ($(strip $(TCSUPPORT_CT_CUSTOMMENU)),)
	cp -f Project/config/menuconfig/CTRelPONMenuConfig Project/config/menuconfig/CT.config
endif
	rm -f Project/config/menuconfig/CTRelPONMenuConfig

#swept unopen source codes,customerized scripts,webpages,configure profile and other extra files
release_swept:
	for FILE in $(UNOPEN_FILE_DELETE) $(OTHER_FILE_DELETE); \
	do\
		echo "******rm -rf $$FILE***********************************";\
		rm -rf $$FILE; \
		echo "******************************************************";\
	done \
	
	if [ $$(ls -A $(BOOTROM_DIR)/ddr_cal/reserved|wc -w) -gt 0 ]; then\
		cp -Rf $(BOOTROM_DIR)/ddr_cal/reserved/* $(BOOTROM_DIR)/ddr_cal/output/.;\
		cp -Rf $(BOOTROM_DIR)/ddr_cal/reserved/spram.c $(BOOTROM_DIR)/ddr_cal/.;\
		rm -rf $(BOOTROM_DIR)/ddr_cal/output/*.c;\
		rm -rf $(BOOTROM_DIR)/ddr_cal/reserved;\
	fi
	if [ $$(ls -A $(BOOTROM_DIR)/ddr_cal_mt7505/reserved|wc -w) -gt 0 ]; then\
		cp -Rf $(BOOTROM_DIR)/ddr_cal_mt7505/reserved/* $(BOOTROM_DIR)/ddr_cal_mt7505/output/.;\
		cp -Rf $(BOOTROM_DIR)/ddr_cal_mt7505/reserved/spram.c $(BOOTROM_DIR)/ddr_cal_mt7505/.;\
		rm -rf $(BOOTROM_DIR)/ddr_cal_mt7505/output/*.c;\
		rm -rf $(BOOTROM_DIR)/ddr_cal_mt7505/reserved;\
	fi
	if [ $$(ls -A $(BOOTROM_DIR)/ddr_cal_en7512/reserved|wc -w) -gt 0 ]; then\
                cp -Rf $(BOOTROM_DIR)/ddr_cal_en7512/reserved/* $(BOOTROM_DIR)/ddr_cal_en7512/output/.;\
                cp -Rf $(BOOTROM_DIR)/ddr_cal_en7512/reserved/spram.c $(BOOTROM_DIR)/ddr_cal_en7512/.;\
                rm -rf $(BOOTROM_DIR)/ddr_cal_en7512/output/*.c;\
                rm -rf $(BOOTROM_DIR)/ddr_cal_en7512/reserved;\
        fi
	find $(APP_PRIVATE_DIR)/etc_script/* -type d  | xargs rm -rf
	if [ $$(ls -A $(APP_PRIVATE_DIR)/etc_script.reserved|wc -w) -gt 0 ]; then\
		cp -rf $(APP_PRIVATE_DIR)/etc_script.reserved/* $(APP_PRIVATE_DIR)/etc_script/ ;\
	fi
ifeq ($(strip $(CUSTOM)),CT)
	rm -rf $(APP_PRIVATE_DIR)/etc_script/ct/CDTool
	rm -rf $(APP_PRIVATE_DIR)/etc_script/ct/eHomeEIWrd_e8bc_20130609.zip
endif
	rm -rf $(APP_PRIVATE_DIR)/etc_script.reserved

	find $(APP_WEBPAGE_DIR)/* -type d | xargs rm -rf
	if [ $$(ls -A $(APP_WEBPAGE_DIR).reserved|wc -w) -gt 0 ]; then\
		cp -rf $(APP_WEBPAGE_DIR).reserved/*  $(APP_WEBPAGE_DIR);\
	fi
	rm -rf $(APP_WEBPAGE_DIR).reserved
	
	find $(APP_PRIVATE_DIR)/led_conf/* -type d | xargs rm -rf
	if [ $$(ls -A $(APP_PRIVATE_DIR)/led_conf.reserved|wc -w) -gt 0 ]; then\
		cp -rf $(APP_PRIVATE_DIR)/led_conf.reserved/* $(APP_PRIVATE_DIR)/led_conf ;\
	fi
	rm -rf $(APP_PRIVATE_DIR)/led_conf.reserved
	
	find $(PROJECT_DIR)/config/menuconfig/ -name "*.config"  -not -name "$(CUSTOM)*" | xargs rm -rf	

	find $(APP_IPTABLES_DIR) -name "*.d"  -not -name "." | xargs rm -rf
	find $(TRUNK_DIR) -name "CVS" | xargs rm -rf
#echo special keys to avoid duplicated release process.	
	echo "TC_BUILD_RELEASECODE=y" > Project/release.chk
	echo "export TC_BUILD_RELEASECODE" >>Project/release.chk
	
	
release_tar:
ifneq ($(strip $(CUSTOM)),)
	tar -czvf ../releasesdk_$(CUSTOM)_`date +%Y%m%d`.tgz .
else
	tar -czvf ../releasesdk_`date +%Y%m%d`.tgz .
endif


#release make for custom, similarly to release_make
customer_release_backup:
ifneq ($(strip $(KERNEL_2_6_36)),)
	for release in ${PROFILE_SELECT} ; \
	do \
		echo "*************************************************"; \
		echo "*************************************************"; \
		echo "*****$$release making and backup in progress********"; \
		echo "*************************************************"; \
		sleep 1;\
		echo "*************************************************"; \
		${MAKE} -f "$(PROJECT_DIR)/MakeFile_Main" PROFILE=$${release} KERNEL_2_6_36=1 CUSTOM=${CUSTOM} clean ; if [ $$? != 0 ];then exit 1;fi;\
		echo "*************************************************"; \
		${MAKE} -f "$(PROJECT_DIR)/MakeFile_Main" PROFILE=$${release} KERNEL_2_6_36=1 CUSTOM=${CUSTOM} CUSTOMERRELEASE=y bootbase kernel modules apps;if [ $$? != 0 ];then exit 1;fi; \
		echo "*************************************************"; \
		${MAKE} -f "$(PROJECT_DIR)/MakeFile_Main" PROFILE=$${release} KERNEL_2_6_36=1 CUSTOM=${CUSTOM} CUSTOMERRELEASE=y customer_release_end; if [ $$? != 0 ];then exit 1;fi;\
		echo "**************************************************"; \
		${MAKE} -f "$(PROJECT_DIR)/MakeFile_Main" PROFILE=$${release} KERNEL_2_6_36=1 CUSTOM=${CUSTOM} clean ;if [ $$? != 0 ];then exit 1;fi; \
		echo "**************************************************"; \
	done
else
	for release in ${PROFILE_SELECT} ; \
	do \
		echo "*************************************************"; \
		echo "*************************************************"; \
		echo "*****$$release making and backup in progress********"; \
		echo "*************************************************"; \
		sleep 1;\
		echo "*************************************************"; \
		${MAKE} -f "$(PROJECT_DIR)/MakeFile_Main" PROFILE=$${release} CUSTOM=${CUSTOM} clean ; if [ $$? != 0 ];then exit 1;fi;\
		echo "*************************************************"; \
		${MAKE} -f "$(PROJECT_DIR)/MakeFile_Main" PROFILE=$${release} CUSTOM=${CUSTOM} CUSTOMERRELEASE=y bootbase kernel modules apps;if [ $$? != 0 ];then exit 1;fi; \
		echo "*************************************************"; \
		${MAKE} -f "$(PROJECT_DIR)/MakeFile_Main" PROFILE=$${release} CUSTOM=${CUSTOM} CUSTOMERRELEASE=y customer_release_end; if [ $$? != 0 ];then exit 1;fi;\
		echo "**************************************************"; \
		${MAKE} -f "$(PROJECT_DIR)/MakeFile_Main" PROFILE=$${release} CUSTOM=${CUSTOM} clean ;if [ $$? != 0 ];then exit 1;fi; \
		echo "**************************************************"; \
	done
endif


customer_release_swept:
	rm -rf $(APP_CMDCI_DIR)
	rm -rf $(APP_ETHCMD_DIR)
	rm -rf $(APP_CWMP_DIR)
	rm -rf $(APP_TCCI_DIR)
	rm -rf $(APP_PRIVATE_DIR)/sendicmp/
	rm -rf $(FILESYSTEM_DIR)/dev/*
	rm -rf $(TRUNK_DIR)/$(MODULES_PRIV_SRC_DIR)/modules/2.6.22.15
	rm -rf $(TRUNK_DIR)/$(MODULES_PRIV_SRC_DIR)/modules/2.6.36
	
	find $(APP_CFG_MANAGER_DIR) -name "*.c"|grep -v 'init.c' |xargs rm -rf
	
	find $(APP_IPTABLES_DIR) -name "*.d" | xargs rm -rf
	echo "CUSTOMER_BUILD_RELEASECODE=y" > Project/customer_release/c_release.chk
	
customer_release_makefile:
	cp $(BOOTROM_DIR)/Makefile_C.release $(TRUNK_DIR)/bootrom/Makefile
	cp $(APP_CFG_MANAGER_DIR)/Makefile_C.release $(APP_CFG_MANAGER_DIR)/Makefile
	
customer_release_tar:
ifneq ($(strip $(CUSTOM)),)
	tar -czvf ../releasesdk_$(CUSTOM)_`date +%Y%m%d`.tgz .
else
	tar -czvf ../releasesdk_`date +%Y%m%d`.tgz .
endif

	
Option_Check:
ifeq ($(strip $(OPTION)),)
	@echo "Command input error!";
	@echo "Command example: sudo make PROFILE=tc3162u Option_Check OPTION=TCSUPPORT_XXX_SUPPORT";
	@exit 1;
endif
	@echo "Checking compile option "${OPTION}" in un-open files now and may last for sevreal minutes, please wait!";
	@for file in $(UNOPEN_FILE_DELETE);\
	do \
		echo "searching in $${file}";\
		find $${file} -name "*.c"|xargs grep -in "${OPTION}" >>"./.tmp_optioncheck"; \
		find $${file} -name "*.h"|xargs grep -in "${OPTION}" >>"./.tmp_optioncheck"; \
		echo ".........................................";\
	done

	@if [ -s "./.tmp_optioncheck" ]; then \
		echo "SORRY,YOUR CHECK HAS FAILED!!!!!!!!";\
		sleep 1;\
		echo "This compile option has been found in following files:";\
		cat "./.tmp_optioncheck";\
		rm -f "./.tmp_optioncheck";\
		exit 1;\
		else \
	echo "Congratulations, your check has PASSED! This compile option has not been found in un-open codes!";\
	echo "..............................................................................................";\
	fi
