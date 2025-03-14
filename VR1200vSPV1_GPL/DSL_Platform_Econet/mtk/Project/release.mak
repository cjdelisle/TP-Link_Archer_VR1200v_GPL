release_end: release_bootrom release_drivers release_apps release_clmp release_script release_webpage release_led_conf
	

release_bootrom:
#ifneq ($(strip $(TCSUPPORT_CPU_MT7510)),)
ifneq ($(strip $(TCSUPPORT_CPU_EN7512) $(TCSUPPORT_CPU_EN7521)),)
	@echo "release Bootrom for MT751221"
	mkdir -p $(BOOTROM_DIR)/ddr_cal_en7512/reserved
	cp -Rf $(BOOTROM_DIR)/ddr_cal_en7512/output/* $(BOOTROM_DIR)/ddr_cal_en7512/reserved/
	cp -Rf $(BOOTROM_DIR)/ddr_cal_en7512/spram.c $(BOOTROM_DIR)/ddr_cal_en7512/reserved/
else
ifneq ($(strip $(TCSUPPORT_CPU_MT7510) $(TCSUPPORT_CPU_MT7520)),)
	@echo "release Bootrom for MT751020"
	mkdir -p $(BOOTROM_DIR)/ddr_cal/reserved
	cp -Rf $(BOOTROM_DIR)/ddr_cal/output/* $(BOOTROM_DIR)/ddr_cal/reserved/
	cp -Rf $(BOOTROM_DIR)/ddr_cal/spram.c $(BOOTROM_DIR)/ddr_cal/reserved/
else
ifneq ($(strip $(TCSUPPORT_CPU_MT7505)),)
	@echo "release Bootrom for MT7505"
	mkdir -p $(BOOTROM_DIR)/ddr_cal_mt7505/reserved
	cp -Rf $(BOOTROM_DIR)/ddr_cal_mt7505/output/* $(BOOTROM_DIR)/ddr_cal_mt7505/reserved/
	cp -Rf $(BOOTROM_DIR)/ddr_cal_mt7505/spram.c $(BOOTROM_DIR)/ddr_cal_mt7505/reserved/
else
	echo "nothing to do!";
endif
endif
endif
release_drivers:
	mkdir -p $(MODULES_PRIV_SRC_DIR)/ko/modules/$(TCPLATFORM)
	cp -rf $(FILESYSTEM_DIR)/lib/modules/* $(MODULES_PRIV_SRC_DIR)/ko/modules/$(TCPLATFORM)/
#	rm -rf $(MODULES_PRIV_SRC_DIR)/tcci/version.c
	mkdir -p $(MODULES_PRIV_SRC_DIR)/obj/$(TCPLATFORM)
	cp -rf $(MODULES_PRIV_SRC_DIR)/tcci/version.o $(MODULES_PRIV_SRC_DIR)/obj/$(TCPLATFORM)/

	
release_apps:
	echo "release apps..."
	mkdir -p $(APP_BINARY_DIR)/$(TCPLATFORM)
#release usb_auto_mount
ifneq ($(strip $(TCSUPPORT_USB_AUTOMOUNT)),)
	cp -rf $(APP_AUTOMOUNT_DIR)/usb_auto_mount  $(APP_BINARY_DIR)/$(TCPLATFORM)
endif

#release auto_mount_dongle
ifneq ($(strip $(TCSUPPORT_USB_3G_DONGLE)),)
	cp -rf $(APP_PRIVATE_DIR)/auto_mount_dongle/auto_mount_dongle   $(APP_BINARY_DIR)/$(TCPLATFORM)
endif


#release restore_linos_info
ifneq ($(strip $(TCSUPPORT_RESTORE_LINOS_INFO)),)
	cp -rf $(APP_RESTORE_LINOS_INFO_DIR)/restore_linos_info  $(APP_BINARY_DIR)/$(TCPLATFORM)
endif

#release skbmgr
ifeq ($(strip $(TCSUPPORT_CT)),)
	cp -rf $(APP_SKB_MANAGER_DIR)/skbmgr  $(APP_BINARY_DIR)/$(TCPLATFORM)	
endif

#Release snmpd
ifneq ($(strip $(TCSUPPORT_SNMP)),)
	cp -rf $(APP_SNMPD_DIR)/agent/snmpd  $(APP_BINARY_DIR)/$(TCPLATFORM)
endif

#Release Zebra & ripd
	cp -rf $(APP_ZEBRA_DIR)/zebra/zebra  $(APP_BINARY_DIR)/$(TCPLATFORM)
	cp -rf $(APP_ZEBRA_DIR)/ripd/ripd  $(APP_BINARY_DIR)/$(TCPLATFORM)
ifneq ($(strip $(TCSUPPORT_IPV6_RIPNG)),)
	cp -rf $(APP_ZEBRA_DIR)/ripngd/ripngd $(APP_BINARY_DIR)/$(TCPLATFORM)
endif
#Release tcapi
	cp -rf $(APP_TCAPI_DIR)/tcapi  $(APP_BINARY_DIR)/$(TCPLATFORM)

#Release upnp-dm
ifneq ($(strip $(TCSUPPORT_CT_UPNP_DM)),)
	cp -rf $(APP_UPNP_DM_DIR)/upnp-dm  $(APP_BINARY_DIR)/$(TCPLATFORM)
endif	

#Release simcard-app
ifneq ($(strip $(TCSUPPORT_CT_SIMCARD_SEPARATION)),)
	cp -rf $(APP_SIM_CARD_DIR)/simCard  $(APP_BINARY_DIR)/$(TCPLATFORM)
	cp -rf $(APP_SIM_CARD_DIR)/simcardapp/simtest  $(APP_BINARY_DIR)/$(TCPLATFORM)
endif

#Release block-process
ifneq ($(strip $(TCSUPPORT_CT_SIMCARD_SEPARATION)),)
	cp -rf $(APP_BLOCK_PROCESS_DIR)/blockProcess  $(APP_BINARY_DIR)/$(TCPLATFORM)
endif

#Release mobile-manager
ifneq ($(strip $(TCSUPPORT_CT_PHONEAPP)),)
	mkdir -p $(MOBILE_MANAGEER_DIR)/$(TCPLATFORM)/src
	mkdir -p $(MOBILE_MANAGEER_DIR)/$(TCPLATFORM)/libmobile
	cp -rf $(MOBILE_MANAGEER_DIR)/src/*.o $(MOBILE_MANAGEER_DIR)/$(TCPLATFORM)/src
	cp -rf $(MOBILE_MANAGEER_DIR)/src/*.h $(MOBILE_MANAGEER_DIR)/$(TCPLATFORM)/src
	cp -rf $(MOBILE_MANAGEER_DIR)/libmobile/* $(MOBILE_MANAGEER_DIR)/$(TCPLATFORM)/libmobile
endif

#Release bandwidth
ifneq ($(strip $(TCSUPPORT_CT_JOYME_BANDWIDTH)),)
	cp -rf $(APP_BANDWIDTH_DIR)/bandwidth  $(APP_BINARY_DIR)/$(TCPLATFORM)
endif

#Release plugin_monitor and usb monitor
ifneq ($(strip $(TCSUPPORT_CT_JOYME)),)
#Release plugin_monitor and usb monitor
	cp -rf $(APP_PLUGIN_MONITOR_DIR)/plugin_monitor  $(APP_BINARY_DIR)/$(TCPLATFORM)
	cp -rf $(USB_MONITOR_DIR)/usb_monitor  $(APP_BINARY_DIR)/$(TCPLATFORM)
#Release app_store ,httpdetect and bundle
	cp -rf $(APP_STORE_DIR)/app_store  $(APP_BINARY_DIR)/$(TCPLATFORM)
	cp -rf $(BUNDLE_CMD_DIR)/bundle $(APP_BINARY_DIR)/$(TCPLATFORM)
	cp -rf $(APP_HTTPDETECT_DIR)/httpdetect  $(APP_BINARY_DIR)/$(TCPLATFORM)
	cp -rf $(BUNDLE_MONITOR_DIR)/bundle_monitor  $(APP_BINARY_DIR)/$(TCPLATFORM)
endif

#Release stb_test
ifneq ($(strip $(TCSUPPORT_CT_STB_TEST)),)
	cp -rf $(APP_STB_TEST_DIR)/stb_test  $(APP_BINARY_DIR)/$(TCPLATFORM)
endif

#Release epon_dummy_detect
ifneq ($(strip $(TCSUPPORT_EPON_DUMMY)),)
	cp -rf $(APP_EPON_DUMMY_DETECT_DIR)/epon_dummy_detect  $(APP_BINARY_DIR)/$(TCPLATFORM)
endif

#Release tcwdog	
	cp -rf $(APP_TCWDOG_DIR)/tcwdog  $(APP_BINARY_DIR)/$(TCPLATFORM)

#Release Driver cmd interface release	
	cp -rf $(APP_CMDCI_DIR)/algcmd  $(APP_BINARY_DIR)/$(TCPLATFORM)
	cp -rf $(APP_CMDCI_DIR)/firewallcmd  $(APP_BINARY_DIR)/$(TCPLATFORM)
	cp -rf $(APP_CMDCI_DIR)/qoscmd	$(APP_BINARY_DIR)/$(TCPLATFORM)
	cp -rf $(APP_CMDCI_DIR)/portbindcmd	$(APP_BINARY_DIR)/$(TCPLATFORM)
ifneq ($(strip $(TCSUPPORT_PRODUCTIONLINE)),)
	cp -rf $(APP_CMDCI_DIR)/prolinecmd $(APP_BINARY_DIR)/$(TCPLATFORM)
endif
ifneq ($(strip $(TCSUPPORT_CT)),)
	cp -rf $(APP_CMDCI_DIR)/accesslimitcmd $(APP_BINARY_DIR)/$(TCPLATFORM)
	cp -rf $(APP_CMDCI_DIR)/atmcmdd $(APP_BINARY_DIR)/$(TCPLATFORM)
endif

#Release igmpproxy
ifneq ($(strip $(TCSUPPORT_IGMP_PROXY_V3)),)
	cp -rf $(APP_IGMPPROXY_DIR)/igmpproxy  $(APP_BINARY_DIR)/$(TCPLATFORM)
endif

#Release hw_nat
ifneq ($(strip $(TCSUPPORT_RA_HWNAT)),)
ifeq ($(strip $(TCSUPPORT_MT7510_FE)),)
	cp -rf $(APP_RA_HWNAT_DIR)/ac  $(APP_BINARY_DIR)/$(TCPLATFORM)
	cp -rf $(APP_RA_HWNAT_DIR)/acl  $(APP_BINARY_DIR)/$(TCPLATFORM)
	cp -rf $(APP_RA_HWNAT_DIR)/mtr  $(APP_BINARY_DIR)/$(TCPLATFORM)
	cp -rf $(APP_RA_HWNAT_DIR)/hw_nat  $(APP_BINARY_DIR)/$(TCPLATFORM)
else
	cp -rf $(APP_RA_HWNAT_7510_DIR)/hw_nat  $(APP_BINARY_DIR)/$(TCPLATFORM)
endif
endif
	
#Release smuxctl
ifneq ($(strip $(TCSUPPORT_SMUX)),)
	cp -rf $(APP_SMUXCTL_DIR)/smuxctl  $(APP_BINARY_DIR)/$(TCPLATFORM)
endif

#Release dms
ifneq ($(strip $(TCSUPPORT_DMS)),)
	cp -rf  $(APP_DMS_DIR)/dlna_output $(APP_BINARY_DIR)/$(TCPLATFORM)/dlna
endif

#Release pwctlcmd
ifneq ($(strip $(TCSUPPORT_CPU_RT65168)),)
	cp -rf $(APP_PWCTLCMD_DIR)/pwctlcmd  $(APP_BINARY_DIR)/$(TCPLATFORM)
endif

#Release ra_menu
ifneq ($(strip $(TCSUPPORT_RA_MENU)),)
	cp -rf $(APP_RA_MENU_DIR)/ra_menu $(APP_BINARY_DIR)/$(TCPLATFORM)/ra_menu
endif

#release backuprestore
ifneq ($(strip $(TCSUPPORT_CT)),)
ifneq ($(strip $(TCSUPPORT_CT_USB_BACKUPRESTORE)),)
	cp -rf $(APP_BACKUPRESTORE_DIR)/backuprestorecmd $(APP_BINARY_DIR)/$(TCPLATFORM)
endif
endif

#release cwmp
ifneq ($(strip $(TCSUPPORT_CWMP)),)
	mkdir -p $(APP_CWMP_DIR)/$(TCPLATFORM)
	$(MAKE) -C $(APP_CWMP_DIR) -f $(APP_CWMP_DIR)/Makefile.release clean
	cp -rf $(APP_CWMP_DIR)/*.o $(APP_CWMP_DIR)/$(TCPLATFORM)
endif

#tcapi library
	mkdir -p $(APP_TCAPILIB_DIR)/$(TCPLATFORM)
	cp -rf $(APP_TCAPILIB_DIR)/*.o  $(APP_TCAPILIB_DIR)/$(TCPLATFORM)
	cp -rf $(APP_TCAPILIB_DIR)/libtcapi.so  $(APP_TCAPILIB_DIR)/$(TCPLATFORM)
	cp -rf $(APP_TCAPILIB_DIR)/libtcapi.so.*  $(APP_TCAPILIB_DIR)/$(TCPLATFORM)
	
#release joyme_lib/ctsgw_lib/bundle_lib
ifneq ($(strip $(TCSUPPORT_CT_JOYME)),)
	mkdir -p $(APP_JOYME_DIR)/$(TCPLATFORM)
	cp -rf $(APP_JOYME_DIR)/joyme.o $(APP_JOYME_DIR)/$(TCPLATFORM)
	
	mkdir -p $(APP_CTSGWLIB_DIR)/$(TCPLATFORM)
	cp -rf $(APP_CTSGWLIB_DIR)/libctsgw.so  $(APP_CTSGWLIB_DIR)/$(TCPLATFORM)
	cp -rf $(APP_CTSGWLIB_DIR)/libctsgw.so.*  $(APP_CTSGWLIB_DIR)/$(TCPLATFORM)
	cp -rf $(APP_CTSGWLIB_DIR)/ctsgw_test/ctsgw_test $(APP_CTSGWLIB_DIR)/$(TCPLATFORM)

	mkdir -p $(APP_BUNDLELIB_DIR)/$(TCPLATFORM)
	cp -rf $(APP_BUNDLELIB_DIR)/libbundle.so  $(APP_BUNDLELIB_DIR)/$(TCPLATFORM)
	cp -rf $(APP_BUNDLELIB_DIR)/libbundle.so.*  $(APP_BUNDLELIB_DIR)/$(TCPLATFORM)
endif

#release cfg_manager.o
#ifeq ($(strip $(TCSUPPORT_CT)),)
	mkdir -p $(APP_CFG_MANAGER_DIR)/$(TCPLATFORM)
	cp -rf $(APP_CFG_MANAGER_DIR)/cfg_manager.o $(APP_CFG_MANAGER_DIR)/$(TCPLATFORM)
#else
#	cp $(APP_PRIVATE_DIR)/cfg_manager_ct/cfg_manager  $(APP_BINARY_DIR)/$(TCPLATFORM)/ 
#endif

#release xpon_libs
ifneq ($(strip $(TCSUPPORT_CPU_MT7520)),)
	mkdir -p $(LIB_XPON_DIR)/$(TCPLATFORM)
ifneq ($(strip $(TCSUPPORT_WAN_EPON)),)
	cp  -rf $(LIB_XPON_DIR)/libepon.so  $(LIB_XPON_DIR)/$(TCPLATFORM)
	cp  -rf $(LIB_XPON_DIR)/libepon.so.*  $(LIB_XPON_DIR)/$(TCPLATFORM)
endif
ifneq ($(strip $(TCSUPPORT_EPON_MAPPING)),)
	cp  -rf $(LIB_XPON_DIR)/libeponmap.so  $(LIB_XPON_DIR)/$(TCPLATFORM)
	cp  -rf $(LIB_XPON_DIR)/libeponmap.so.*  $(LIB_XPON_DIR)/$(TCPLATFORM)
endif
ifneq ($(strip $(TCSUPPORT_GPON_MAPPING)),)
	cp  -rf $(LIB_XPON_DIR)/libgponmap.so $(LIB_XPON_DIR)/$(TCPLATFORM)
	cp  -rf $(LIB_XPON_DIR)/libgponmap.so.* $(LIB_XPON_DIR)/$(TCPLATFORM)
endif
ifneq ($(strip $(TCSUPPORT_PON_VLAN)),)
	cp  -rf $(LIB_XPON_DIR)/libpon_vlan.so  $(LIB_XPON_DIR)/$(TCPLATFORM)
	cp  -rf $(LIB_XPON_DIR)/libpon_vlan.so.*  $(LIB_XPON_DIR)/$(TCPLATFORM)
endif
ifneq ($(strip $(TCSUPPORT_PON_MAC_FILTER)),)
	cp  -rf $(LIB_XPON_DIR)/libpon_mac_filter.so  $(LIB_XPON_DIR)/$(TCPLATFORM)
	cp  -rf $(LIB_XPON_DIR)/libpon_mac_filter.so.*  $(LIB_XPON_DIR)/$(TCPLATFORM)
endif
ifneq ($(strip $(TCSUPPORT_XPON_IGMP)),)
	cp  -rf $(LIB_XPON_DIR)/libxpon_igmp.so  $(LIB_XPON_DIR)/$(TCPLATFORM)
	cp  -rf $(LIB_XPON_DIR)/libxpon_igmp.so.*  $(LIB_XPON_DIR)/$(TCPLATFORM)
endif
endif

#release oam
ifneq ($(strip $(TCSUPPORT_EPON_OAM)),)
	mkdir -p $(APP_EPON_OAM_DIR)/$(TCPLATFORM)
	cp -rf $(APP_EPON_OAM_DIR)/epon_oam.o $(APP_EPON_OAM_DIR)/$(TCPLATFORM)
	cp -rf $(APP_EPON_OAM_DIR)/epon_oam_ext.o $(APP_EPON_OAM_DIR)/$(TCPLATFORM)
	cp -rf $(APP_EPON_OAM_DIR)/epon_oam_dbg_srv.o $(APP_EPON_OAM_DIR)/$(TCPLATFORM)
	cp -rf $(APP_EPON_OAM_DIR)/epon_oam_util.o $(APP_EPON_OAM_DIR)/$(TCPLATFORM)
	cp -rf $(APP_EPON_OAM_DIR)/epon_oam_timer.o $(APP_EPON_OAM_DIR)/$(TCPLATFORM)
endif

#release omci
ifneq ($(strip $(TCSUPPORT_OMCI)),)
	mkdir -p $(APP_OMCI_DIR)/$(TCPLATFORM)
	cp -rf $(APP_OMCI_DIR)/omci.o $(APP_OMCI_DIR)/$(TCPLATFORM)
	cp -rf $(APP_OMCI_DIR)/omci_dbg_srv.o $(APP_OMCI_DIR)/$(TCPLATFORM)
	cp -rf $(APP_OMCI_DIR)/omci_general_func.o $(APP_OMCI_DIR)/$(TCPLATFORM)
	cp -rf $(APP_OMCI_DIR)/omci_message_handle.o $(APP_OMCI_DIR)/$(TCPLATFORM)
	cp -rf $(APP_OMCI_DIR)/omci_timer.o $(APP_OMCI_DIR)/$(TCPLATFORM)
	cp -rf $(APP_OMCI_DIR)/omci_utily.o $(APP_OMCI_DIR)/$(TCPLATFORM)
endif



#release IgdClmp.o ad IgdStubsClmp.o with and with out WLAN option.	
TC_CFLAGS_NO_WLAN=$(patsubst -DTCSUPPORT_WLAN,,$(TC_CFLAGS))
export TC_CFLAGS_NO_WLAN

release_clmp:
	echo "release IgdClmp and IgdStubsClmp..."
ifneq ($(strip $(TCSUPPORT_CWMP)),)
	mkdir -p $(APP_CWMP_DIR)/$(TCPLATFORM)/
	$(MAKE) -C $(APP_CWMP_DIR) clean
	$(MAKE) -C $(APP_CWMP_DIR) TC_CFLAGS="$(TC_CFLAGS_NO_WLAN)"
	cp -f $(APP_CWMP_DIR)/IgdClmp.o $(APP_CWMP_DIR)/$(TCPLATFORM)/IgdClmp_noWlan.o
	cp -f $(APP_CWMP_DIR)/IgdStubsClmp.o $(APP_CWMP_DIR)/$(TCPLATFORM)/IgdStubsClmp_noWlan.o
endif	

release_script:
	echo "release etc_script now..."
	mkdir -p $(APP_PRIVATE_DIR)/etc_script.reserved/
	


	
#if !defined(TCSUPPORT_TTNET)
ifneq ($(strip $(TCSUPPORT_TTNET)),)
	mkdir -p $(APP_PRIVATE_DIR)/etc_script.reserved/ttnet/
	cp -rf $(APP_PRIVATE_DIR)/etc_script/ttnet/* $(APP_PRIVATE_DIR)/etc_script.reserved/ttnet/
endif
#endif

ifneq ($(strip $(TCSUPPORT_CPU_MT7520)),)
	mkdir -p $(APP_PRIVATE_DIR)/etc_script.reserved/pon/
	cp -rf $(APP_PRIVATE_DIR)/etc_script/pon/* $(APP_PRIVATE_DIR)/etc_script.reserved/pon/
endif


release_webpage:
	echo "release webpage...."
#copy tc webpage by default
	mkdir -p $(APP_WEBPAGE_DIR).reserved/Router/tc/
	cp -rf  $(APP_ROUTE_WEBPAGE_DIR)  $(APP_WEBPAGE_DIR).reserved/Router/tc/

ifneq ($(strip $(TCSUPPORT_GENERAL_MULTILANGUAGE)),)
	mkdir -p $(APP_WEBPAGE_DIR).reserved/Router/ml
	cp -rf $(APP_WEBPAGE_DIR)/Router/ml/boaroot $(APP_WEBPAGE_DIR).reserved/Router/ml
endif






#copy led conf file, this "interface is left for future use"
release_led_conf:
	echo "copy led conf..."
	mkdir -p $(APP_PRIVATE_DIR)/led_conf.reserved	

release_chk:
	if test -d $(APP_PRIVATE_DIR)/TR69_64/clmp; \
	then echo "Origin Src Code"; \
	else echo "Release Src Code, you can not release second time!"; exit 1;\
	fi

ifneq ($(strip $(TCSUPPORT_FON)),)
release_fon:
	mkdir -p $(APP_FONSMCD_DIR)/libs/fonsmcd/$(TCPLATFORM)
	cp -rf $(APP_FONSMCD_DIR)/src/fonsmcd/fon_conf $(APP_FONSMCD_DIR)/libs/fonsmcd/
	cp $(APP_FONSMCD_DIR)/src/fonsmcd/fonsmcd $(APP_FONSMCD_DIR)/libs/fonsmcd/$(TCPLATFORM)/

	cd $(APP_FONSMCD_DIR)/src/coova && ./bootstrap && chmod 777 configure
	cd $(APP_FONSMCD_DIR)/src/coova && ./configure --host=mips-linux --prefix=/usr --sysconfdir=/etc --localstatedir=/var --enable-ipwhitelist --enable-uamdomainfile --enable-redirdnsreq --enable-binstatusfile --disable-accounting-onoff CFLAGS="-Os -Wall -mips1"
	$(MAKE) -C $(APP_FONSMCD_DIR)/src/coova
	$(STRIP) --strip-unneeded -R .comment $(APP_FONSMCD_DIR)/src/coova/src/.libs/chilli
	$(STRIP) --strip-unneeded -R .comment $(APP_FONSMCD_DIR)/src/coova/src/.libs/chilli_opt
	$(STRIP) --strip-unneeded -R .comment $(APP_FONSMCD_DIR)/src/coova/src/.libs/chilli_query
	$(STRIP) --strip-unneeded -R .comment $(APP_FONSMCD_DIR)/src/coova/src/.libs/chilli_radconfig
	$(STRIP) --strip-unneeded -R .comment $(APP_FONSMCD_DIR)/src/coova/src/.libs/chilli_response
	$(STRIP) --strip-unneeded -R .comment $(APP_FONSMCD_DIR)/src/coova/src/.libs/libchilli.so*
	$(STRIP) --strip-unneeded -R .comment $(APP_FONSMCD_DIR)/src/coova/bstring/.libs/libbstring.so*
	mkdir -p $(APP_FONSMCD_DIR)/libs/coova/$(TCPLATFORM)/model_a/
	cp $(APP_FONSMCD_DIR)/src/coova/src/.libs/chilli $(APP_FONSMCD_DIR)/libs/coova/$(TCPLATFORM)/model_a/
	cp $(APP_FONSMCD_DIR)/src/coova/src/.libs/chilli_opt $(APP_FONSMCD_DIR)/libs/coova/$(TCPLATFORM)/model_a/
	cp $(APP_FONSMCD_DIR)/src/coova/src/.libs/chilli_query $(APP_FONSMCD_DIR)/libs/coova/$(TCPLATFORM)/model_a/
	cp $(APP_FONSMCD_DIR)/src/coova/src/.libs/chilli_radconfig $(APP_FONSMCD_DIR)/libs/coova/$(TCPLATFORM)/model_a/
	cp $(APP_FONSMCD_DIR)/src/coova/src/.libs/chilli_response $(APP_FONSMCD_DIR)/libs/coova/$(TCPLATFORM)/model_a/
	cp $(APP_FONSMCD_DIR)/src/coova/src/.libs/libchilli.so* $(APP_FONSMCD_DIR)/libs/coova/$(TCPLATFORM)/model_a/
	cp $(APP_FONSMCD_DIR)/src/coova/bstring/.libs/libbstring.so* $(APP_FONSMCD_DIR)/libs/coova/$(TCPLATFORM)/
	mkdir -p $(APP_FONSMCD_DIR)/libs/coova/files_fon/
	cp $(APP_FONSMCD_DIR)/src/coova/files_fon/localusers $(APP_FONSMCD_DIR)/libs/coova/files_fon/

	cd $(APP_FONSMCD_DIR)/src/fon-api && ./bootstrap && chmod 777 configure
	cd $(APP_FONSMCD_DIR)/src/fon-api && ./configure --host=mips-linux CFLAGS="-Os -Wall -mips1"
	$(MAKE) -C $(APP_FONSMCD_DIR)/src/fon-api
	$(STRIP) --strip-unneeded -R .comment $(APP_FONSMCD_DIR)/src/fon-api/.libs/fonctl
	$(STRIP) --strip-unneeded -R .comment $(APP_FONSMCD_DIR)/src/fon-api/.libs/fon-api.so
	mkdir -p $(APP_FONSMCD_DIR)/libs/fon-api/$(TCPLATFORM)/model_a
	cp $(APP_FONSMCD_DIR)/src/fon-api/.libs/fonctl $(APP_FONSMCD_DIR)/libs/fon-api/$(TCPLATFORM)/model_a/
	cp $(APP_FONSMCD_DIR)/src/fon-api/.libs/fon-api.so $(APP_FONSMCD_DIR)/libs/fon-api/$(TCPLATFORM)/model_a/
	$(MAKE) -C $(APP_FONSMCD_DIR)/src/coova clean
	$(MAKE) -C $(APP_FONSMCD_DIR)/src/fon-api clean
			
ifneq ($(strip $(TCSUPPORT_FON_MODEL_B)),)	
	cd $(APP_FONSMCD_DIR)/src/coova && ./bootstrap && chmod 777 configure
	cd $(APP_FONSMCD_DIR)/src/coova && ./configure --host=mips-linux --prefix=/usr --sysconfdir=/etc --localstatedir=/var --enable-ipwhitelist --enable-uamdomainfile --enable-redirdnsreq --enable-binstatusfile --disable-accounting-onoff CFLAGS="-DTCSUPPORT_FON_MODEL_B -Os -Wall -mips1" LDFLAGS="-lrt $(TRUNK_DIR)/apps/private/lib/libtcapi.so"
	$(MAKE) -C $(APP_FONSMCD_DIR)/src/coova
	$(STRIP) --strip-unneeded -R .comment $(APP_FONSMCD_DIR)/src/coova/src/.libs/chilli
	$(STRIP) --strip-unneeded -R .comment $(APP_FONSMCD_DIR)/src/coova/src/.libs/chilli_opt
	$(STRIP) --strip-unneeded -R .comment $(APP_FONSMCD_DIR)/src/coova/src/.libs/chilli_query
	$(STRIP) --strip-unneeded -R .comment $(APP_FONSMCD_DIR)/src/coova/src/.libs/chilli_radconfig
	$(STRIP) --strip-unneeded -R .comment $(APP_FONSMCD_DIR)/src/coova/src/.libs/chilli_response
	$(STRIP) --strip-unneeded -R .comment $(APP_FONSMCD_DIR)/src/coova/src/.libs/libchilli.so*
	$(STRIP) --strip-unneeded -R .comment $(APP_FONSMCD_DIR)/src/coova/bstring/.libs/libbstring.so*
	mkdir -p $(APP_FONSMCD_DIR)/libs/coova/$(TCPLATFORM)/model_b/
	cp $(APP_FONSMCD_DIR)/src/coova/src/.libs/chilli $(APP_FONSMCD_DIR)/libs/coova/$(TCPLATFORM)/model_b/
	cp $(APP_FONSMCD_DIR)/src/coova/src/.libs/chilli_opt $(APP_FONSMCD_DIR)/libs/coova/$(TCPLATFORM)/model_b/
	cp $(APP_FONSMCD_DIR)/src/coova/src/.libs/chilli_query $(APP_FONSMCD_DIR)/libs/coova/$(TCPLATFORM)/model_b/
	cp $(APP_FONSMCD_DIR)/src/coova/src/.libs/chilli_radconfig $(APP_FONSMCD_DIR)/libs/coova/$(TCPLATFORM)/model_b/
	cp $(APP_FONSMCD_DIR)/src/coova/src/.libs/chilli_response $(APP_FONSMCD_DIR)/libs/coova/$(TCPLATFORM)/model_b/
	cp $(APP_FONSMCD_DIR)/src/coova/src/.libs/libchilli.so* $(APP_FONSMCD_DIR)/libs/coova/$(TCPLATFORM)/model_b/
	
	cd $(APP_FONSMCD_DIR)/src/fon-api && ./bootstrap && chmod 777 configure
	cd $(APP_FONSMCD_DIR)/src/fon-api && ./configure --host=mips-linux CFLAGS="-Os -Wall -mips1" LDFLAGS="-lrt $(TRUNK_DIR)/apps/private/lib/libtcapi.so" 
	$(MAKE) -C $(APP_FONSMCD_DIR)/src/fon-api
	$(STRIP) --strip-unneeded -R .comment $(APP_FONSMCD_DIR)/src/fon-api/.libs/fonctl
	$(STRIP) --strip-unneeded -R .comment $(APP_FONSMCD_DIR)/src/fon-api/.libs/fon-api.so
	mkdir -p $(APP_FONSMCD_DIR)/libs/fon-api/$(TCPLATFORM)/model_b
	cp $(APP_FONSMCD_DIR)/src/fon-api/.libs/fonctl $(APP_FONSMCD_DIR)/libs/fon-api/$(TCPLATFORM)/model_b/
	cp $(APP_FONSMCD_DIR)/src/fon-api/.libs/fon-api.so $(APP_FONSMCD_DIR)/libs/fon-api/$(TCPLATFORM)/model_b/
	$(MAKE) -C $(APP_FONSMCD_DIR)/src/coova clean
	$(MAKE) -C $(APP_FONSMCD_DIR)/src/fon-api clean	
endif
endif

ifneq ($(strip $(TCSUPPORT_FON_V2)),)
release_fonV2:
	echo "Build hotspotd"
	mkdir -p $(APP_PUBLIC_DIR)/hotspot/$(TCPLATFORM)
	$(MAKE) -C $(APP_FON_HOTSPOT_DIR) MODULES="fonsmc fonapi radius radconf fonctl tunnel" LDFLAGS="-lrt $(TRUNK_DIR)/apps/private/lib/libtcapi.so -lrt $(TRUNK_DIR)/apps/public/pcre-8.32/.libs/libpcre.so" 
	if test -e $(APP_FON_HOTSPOT_DIR)/hotspotd; \
	then \
		$(STRIP) --strip-unneeded -R .comment $(APP_FON_HOTSPOT_DIR)/hotspotd; \
		cp -rf $(APP_FON_HOTSPOT_DIR)/hotspotd $(APP_PUBLIC_DIR)/hotspot/$(TCPLATFORM); \
	fi
	
	cp -rf $(APP_FON_HOTSPOT_DIR)/conf/ $(APP_PUBLIC_DIR)/hotspot
	$(MAKE) -C $(APP_FON_HOTSPOT_DIR) clean
endif