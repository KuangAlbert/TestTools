#! /bin/bash
#make
cd tmp/deploy/images/mt2701-ds03h/
if [ -z $1 ];then
	echo "------------------------------"
	echo "Download boot kernel rootfs"
	echo "------------------------------"
	fastboot erase nor0
	fastboot flash nor0 MBR_NOR
	fastboot flash UBOOT lk.img
	fastboot flash TEE1 tz.img
	fastboot flash BOOTIMG boot.img
	fastboot flash ROOTFS rootfs.ext4
	fastboot flash STATE state.ext4
elif [ $1 == 0 ];then
	echo "------------------------------"
	echo "Download uboot"
	echo "------------------------------"
	fastboot erase nor0
	fastboot flash nor0 MBR_NOR
	fastboot flash UBOOT lk.img
	fastboot flash TEE1 tz.img

	fastboot flash BOOTIMG boot.img
elif [ $1 == 1 ];then
	echo "------------------------------"
	echo "Download kernel"
	echo "------------------------------"
	fastboot flash BOOTIMG boot.img
elif [ $1 == 2 ];then
	echo "------------------------------"
	echo "Download roootfs"
	echo "------------------------------"
	fastboot flash ROOTFS rootfs.ext4
	fastboot flash STATE state.ext4
elif [ $1 == 3 ];then
	echo "------------------------------"
	echo "Download recovery"
	echo "------------------------------"
	fastboot flash RECOVERY recovery.img
	fastboot flash RECOVERYROOTFS recovery.ext4
fi

cd ../../../../
exit 0
#gnome-terminal --maximize -t "ds03h" -x bash  -c "cd /media//work/workplace/ds03h;source env_setup.sh;exec bash;"
