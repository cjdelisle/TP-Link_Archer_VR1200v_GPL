deps_config := \
	ParallelBuildConfig \
	CustomConfig \
	Config.in

include/config/auto.conf: \
	$(deps_config)

$(deps_config): ;
