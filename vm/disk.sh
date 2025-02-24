#!/bin/sh

loopdev="/dev/loop30"
loopdev_part="/dev/loop30p1"
point="${HOME}/src/proto-nvlink/vm/part1"
code="${HOME}/src/proto-nvlink/src/sw"

[ -z $IMG ] && echo "Usage:\tIMG=<file.img> <command>" && exit 1

# se pueden hacer los tres pasos a la vez con "./disk.sh -iur"

while getopts "iur" opt; do
	case "$opt" in
		i)
			sudo losetup -P $loopdev $IMG
			sudo mount $loopdev_part $point
			;;
		u)
			sudo cp $code/kernel/pnvl.ko $point
			sudo cp $code/userspace/master $point
			sudo cp $code/userspace/chiplet $point
			;;
		r)
			sudo umount $point
			sudo losetup -d $loopdev
			;;
	esac
done
