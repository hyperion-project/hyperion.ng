#!/bin/sh

RPI_ROOTFS=/home/tvdzwan/hummingboard/rootfs
IMX6_ROOTFS=/home/tvdzwan/hummingboard/rootfs
outfile=hyperion.deps.openelec-imx6.tar.gz

tar --create --verbose --gzip --absolute-names --show-transformed-names --dereference \
	--file "$outfile" \
	"$IMX6_ROOTFS/usr/lib/arm-linux-gnueabihf/libaudio.so.2" \
	"$IMX6_ROOTFS/usr/lib/arm-linux-gnueabihf/libffi.so.6" \
	"$IMX6_ROOTFS/usr/lib/arm-linux-gnueabihf/libICE.so.6" \
	"$IMX6_ROOTFS/lib/arm-linux-gnueabihf/libpcre.so.3" \
	"$IMX6_ROOTFS/usr/lib/arm-linux-gnueabihf/libpng12.so.0" \
	"$IMX6_ROOTFS/usr/lib/arm-linux-gnueabihf/libQtCore.so.4" \
	"$IMX6_ROOTFS/usr/lib/arm-linux-gnueabihf/libQtGui.so.4" \
	"$IMX6_ROOTFS/usr/lib/arm-linux-gnueabihf/libQtNetwork.so.4" \
	"$IMX6_ROOTFS/usr/lib/arm-linux-gnueabihf/libSM.so.6" \
	"$IMX6_ROOTFS/usr/lib/arm-linux-gnueabihf/libX11.so.6" \
	"$IMX6_ROOTFS/usr/lib/arm-linux-gnueabihf/libXau.so.6" \
	"$IMX6_ROOTFS/usr/lib/arm-linux-gnueabihf/libxcb.so.1" \
	"$IMX6_ROOTFS/usr/lib/arm-linux-gnueabihf/libXdmcp.so.6" \
	"$IMX6_ROOTFS/usr/lib/arm-linux-gnueabihf/libXext.so.6" \
	"$IMX6_ROOTFS/usr/lib/arm-linux-gnueabihf/libXrender.so.1" \
	"$IMX6_ROOTFS/usr/lib/arm-linux-gnueabihf/libXt.so.6" \
	"./openelec/hyperiond.sh" \
	"./openelec/hyperion-v4l2.sh" \
	"./openelec/hyperion-remote.sh"

