#**************************************************************************** 
# 
#  Copyright(c) Shenzhen TP-Link Corporation Limited 
#  All Rights Reserved 
#  DingCheng <DingCheng@tp-link.com.cn> 
# 
#***************************************************************************
#!/bin/sh
command=""
build_dir=$(pwd)

# 1.parse args
for arg in $@
do
case "$arg" in
	MODEL=*	 ) m=${arg##*MODEL=};; #command=$command" "$arg;;
	*	 ) command=$command" "$arg;;
esac
done

# 2.check MODEL
if [ "$m" = "" ]; then
	echo "You have to define MODEL"
	exit 1;
fi

# 3.build all
cd $build_dir/../apps/public/ipsectools && touch * && cd -
cd $build_dir/../apps/public/libusb-1.0.8 && touch * && cd -
cd $build_dir/../apps/public/ipsectools/src/racoon && touch * && cd -
chmod 777 -R $build_dir/../../../VR1200vSPV1_GPL
make MODEL=$m env_build boot_build kernel_build modules_build apps_build fs_build image_build

exit 0
