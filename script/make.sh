#! /bin/bash
#make
cd tmp/deploy/images/mt2701-ds03h/
if [ -z $1 ];then
	echo "Download all!!!"
	fastboot erase nor0
	fastboot flash nor0 MBR_NOR
	fastboot flash UBOOT lk.img
	fastboot flash TEE1 tz.img
	fastboot flash BOOTIMG boot.img
	fastboot flash ROOTFS rootfs.ext4
	fastboot flash STATE state.ext4	
elif [ $1 == 0 ];then
	echo "Download uboot and kernel!!!"
	fastboot erase nor0
	fastboot flash nor0 MBR_NOR
	fastboot flash UBOOT lk.img
	fastboot flash TEE1 tz.img

	fastboot flash BOOTIMG boot.img
elif [ $1 == 1 ];then
	echo "Download linux!!!"
	fastboot flash BOOTIMG boot.img
elif [ $1 == 2 ];then
	echo "Download roootfs!!!"
	fastboot flash ROOTFS rootfs.ext4
	fastboot flash STATE state.ext4	
fi

cd ../../../../
exit 0
#gnome-terminal --maximize -t "ds03h" -x bash  -c "cd /media//work/workplace/ds03h;source env_setup.sh;exec bash;"
