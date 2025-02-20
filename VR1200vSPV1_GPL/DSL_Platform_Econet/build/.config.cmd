deps_config := \
	/home/dev/linux_mtk_VR410v1/build/../build/sysdeps/linux/Config.in

.config include/config.h: $(deps_config)

$(deps_config):
