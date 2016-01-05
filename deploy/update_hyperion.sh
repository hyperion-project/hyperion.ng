#!/bin/sh

# Script for downloading and installing the latest Hyperion release

# Make sure /sbin is on the path (for service to find sub scripts)
PATH="/sbin:$PATH"

# Find out if we are on OpenElec
OS_OPENELEC=`cat /etc/issue | grep -m 1 OpenELEC | wc -l`

# Find out if its an imx6 device
CPU_RPI=`cat /proc/cpuinfo | grep RPI | wc -l`
CPU_IMX6=`cat /proc/cpuinfo | grep i.MX6 | wc -l`
CPU_WETEK=`cat /proc/cpuinfo | grep Amlogic | wc -l`
CPU_X64=`uname -m | grep x86_64 | wc -l`
CPU_X32=`uname -m | grep x86_32 | wc -l`
# Check that we have a known configuration
if [[ $CPU_RPI -ne 1 && $CPU_IMX6 -ne 1 && $CPU_WETEK -ne 1 && $CPU_X64 -ne 1 && $CPU_X32 -ne 1 ]]; then
	echo 'CPU information does not match any known releases'
	exit
fi

# check which init script we should use
USE_INITCTL=`which /sbin/initctl | wc -l`
USE_SERVICE=`which /usr/sbin/service | wc -l`

# Make sure that the boblight daemon is no longer running
BOBLIGHT_PROCNR=$(pidof boblightd | wc -l)
if [ $BOBLIGHT_PROCNR -eq 1 ]; then
	echo 'Found running instance of boblight. Please stop boblight via XBMC menu before installing hyperion'
	exit
fi

# Stop hyperion daemon if it is running
# Start the hyperion daemon
if [ $USE_INITCTL -eq 1 ]; then
	/sbin/initctl stop hyperion
elif [ $USE_SERVICE -eq 1 ]; then
	/usr/sbin/service hyperion stop
fi

# Select the appropriate release
HYPERION_ADDRESS=https://raw.githubusercontent.com/tvdzwan/hyperion/master/deploy
if [ $CPU_RPI -eq 1 ]; then
	HYPERION_RELEASE=$HYPERION_ADDRESS/hyperion_rpi.tar.gz
	OE_DEPENDECIES=$HYPERION_ADDRESS/hyperion.deps.openelec-rpi.tar.gz
elif [ $CPU_IMX6 -eq 1 ]; then
	HYPERION_RELEASE=$HYPERION_ADDRESS/hyperion_imx6.tar.gz
	OE_DEPENDECIES=$HYPERION_ADDRESS/hyperion.deps.openelec-imx6.tar.gz
elif [ $CPU_WETEK -eq 1 ]; then
	HYPERION_RELEASE=$HYPERION_ADDRESS/hyperion_wetek.tar.gz
	OE_DEPENDECIES=$HYPERION_ADDRESS/hyperion.deps.openelec-rpi.tar.gz
elif [ $CPU_X64 -eq 1 ]; then
	HYPERION_RELEASE=$HYPERION_ADDRESS/hyperion_x64.tar.gz
	OE_DEPENDECIES=$HYPERION_ADDRESS/hyperion.deps.openelec-x64.tar.gz
elif [ $CPU_X32 -eq 1 ]; then
	HYPERION_RELEASE=$HYPERION_ADDRESS/hyperion_x32.tar.gz
	OE_DEPENDECIES=$HYPERION_ADDRESS/hyperion.deps.openelec-x32.tar.gz
else
	echo "Target platform unknown"
	exit
fi

# Get and extract the Hyperion binaries and effects
echo 'Downloading hyperion'
if [ $OS_OPENELEC -eq 1 ]; then
	# OpenELEC has a readonly file system. Use alternative location
	curl -L --get $HYPERION_RELEASE | tar -C /storage -xz
	curl -L --get $OE_DEPENDECIES | tar -C /storage/hyperion/bin -xz

	# modify the default config to have a correct effect path
	sed -i 's:/opt:/storage:g' /storage/hyperion/config/hyperion.config.json

	# /storage/.config is available as samba share. A symbolic link would not be working
	false | cp -i /storage/hyperion/config/hyperion.config.json /storage/.config/hyperion.config.json 2>/dev/null
else
	wget $HYPERION_RELEASE -O - | tar -C /opt -xz

	# create links to the binaries
	ln -fs /opt/hyperion/bin/hyperiond /usr/bin/hyperiond
	ln -fs /opt/hyperion/bin/hyperion-remote /usr/bin/hyperion-remote
	ln -fs /opt/hyperion/bin/hyperion-v4l2 /usr/bin/hyperion-v4l2

# Copy a link to the hyperion configuration file to /etc
	ln -s /opt/hyperion/config/hyperion.config.json /etc/hyperion.config.json
fi

# Copy the service control configuration to /etc/int
if [ $USE_INITCTL -eq 1 ]; then
	echo 'Installing initctl script'
	wget -N https://raw.githubusercontent.com/tvdzwan/hyperion/master/deploy/hyperion.conf -P /etc/init/
	initctl reload-configuration
elif [ $USE_SERVICE -eq 1 ]; then
	echo 'Installing startup script in init.d'
	# place startup script in init.d and add it to upstart
	ln -fs /opt/hyperion/init.d/hyperion.init.sh /etc/init.d/hyperion
	chmod +x /etc/init.d/hyperion
	update-rc.d hyperion defaults 98 02
elif [ $OS_OPENELEC -eq 1 ]; then
	# only add to start script if hyperion is not present yet
	if [ `cat /storage/.config/autostart.sh 2>/dev/null | grep hyperiond | wc -l` -eq 0 ]; then
		echo 'Adding Hyperion to autostart script'
		echo "/storage/hyperion/bin/hyperiond.sh /storage/.config/hyperion.config.json > /dev/null 2>&1 &" >> /storage/.config/autostart.sh
		chmod +x /storage/.config/autostart.sh
	fi
fi

# Start the hyperion daemon
if [ $USE_INITCTL -eq 1 ]; then
	/sbin/initctl start hyperion
elif [ $USE_SERVICE -eq 1 ]; then
	/usr/sbin/service hyperion start
fi

