#!/bin/sh
START_ADDR=0x80002000

kernel=$1/mtk_en7512_kernel/linux.7z
rootfs=$1/rootfs.$2

size_k=$(stat -c%s "$kernel")
size_r=$(stat -c%s "$rootfs")

buildDay=$(date +%y%m%d)

cat $kernel $rootfs > $1/dimage_$2/tclinux_$2
echo `./trx -k $size_k -r $size_r -u $START_ADDR -f $1/dimage_$2/tclinux_$2 -o $1/dimage_$2/$2_flash\($buildDay\).bin -c ./trx_config`

test -e $1/dimage_$2/tclinux_$2 && rm -fr $1/dimage_$2/tclinux_$2