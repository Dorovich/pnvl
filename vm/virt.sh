#!/bin/sh

server=off
port=9995
disk="vda-cli.img"
name="Linux 6.6.72 on riscv64 PNVL-client"

while getopts "dsp:" opt; do
	case "$opt" in
		d)
			flags="-s"
			;;
		s)
			server=on
			disk="vda.img"
			name="Linux 6.6.72 on riscv64 PNVL-server"
			;;
		p)
			port=$OPTARG
			;;
	esac
done
shift $((OPTIND-1))

#qemu-system-riscv64 \
~/src/proto-nvlink/qemu/build/qemu-system-riscv64 \
	-machine virt \
	-cpu rv64 \
	-m 256M \
	-smp 1 \
	-drive file=$disk,format=raw,if=none,id=hd0 \
	-device virtio-blk-device,drive=hd0 \
	-kernel ~/src/proto-nvlink/linux-6.6.72/arch/riscv/boot/Image \
	-append "nokaslr root=/dev/vda1 rw console=ttyS0" \
	-device pnvl,server_mode=$server,port=$port \
	-monitor stdio \
	-name "$name" \
	$flags
