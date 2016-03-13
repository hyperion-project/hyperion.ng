#!/bin/sh
# Script for downloading and installing the latest Hyperion release

# Make sure /sbin is on the path (for service to find sub scripts)
PATH="/sbin:$PATH"

#Check if HyperCon is logged in as root
if [ $(id -u) != 0 ] && [ "$1" = "HyperConInstall" ]; then
		echo '---> Critical Error: Please connect as user "root" through HyperCon' 
		echo '---> We need admin privileges to install/update your Hyperion! -> abort'
		exit 1
fi

#Check, if script is running as root
if [ $(id -u) != 0 ]; then
		echo '---> Critical Error: Please run the script as root (sudo sh ./install_hyperion.sh) -> abort' 
		exit 1
fi

#Welcome message
echo '*******************************************************************************' 
echo 'This script will install/update Hyperion and it´s services' 
echo 'Version 0.1' 
echo '*******************************************************************************'

# Find out if we are on OpenElec / OSMC
OS_OPENELEC=`grep -m1 -c OpenELEC /etc/issue`
OS_OSMC=`grep -m1 -c OSMC /etc/issue`

# Find out if its an imx6 device
CPU_RPI=`grep -m1 -c 'BCM2708\|BCM2709\|BCM2710' /proc/cpuinfo`
CPU_IMX6=`grep -m1 -c i.MX6 /proc/cpuinfo`
CPU_WETEK=`grep -m1 -c Amlogic /proc/cpuinfo`
CPU_X64=`uname -m | grep x86_64 | wc -l`
CPU_X32=`uname -m | grep 'x86_32\|i686' | wc -l`
# Check that we have a known configuration
if [ $CPU_RPI -ne 1 ] && [ $CPU_IMX6 -ne 1 ] && [ $CPU_WETEK -ne 1 ] && [ $CPU_X64 -ne 1 ] && [ $CPU_X32 -ne 1 ]; then
	echo '---> Critical Error: CPU information does not match any known releases -> abort'
	exit 1
fi
#Check, if year equals 1970
DATE=$(date +"%Y")
if [ "$DATE" -le "2015" ]; then
        echo "---> Critical Error: Please update your systemtime (Year of your system: ${DATE}) -> abort"
        exit 1
fi

# check which init script we should use
USE_SYSTEMD=`grep -m1 -c systemd /proc/1/comm`
USE_INITCTL=`which /sbin/initctl | wc -l`
USE_SERVICE=`which /usr/sbin/service | wc -l`

# Make sure that the boblight daemon is no longer running
BOBLIGHT_PROCNR=$(pidof boblightd | wc -l)
if [ $BOBLIGHT_PROCNR -eq 1 ]; then
	echo '---> Critical Error: Found running instance of boblight. Please stop boblight via XBMC menu before installing hyperion -> abort'
	exit 1
fi

# Stop hyperion daemon if it is running
echo '---> Stop Hyperion, if necessary'
if [ $OS_OPENELEC -eq 1 ]; then
    killall hyperiond 2>/dev/null
elif [ $USE_INITCTL -eq 1 ]; then
	/sbin/initctl stop hyperion 2>/dev/null
elif [ $USE_SERVICE -eq 1 ]; then
	/usr/sbin/service hyperion stop 2>/dev/null
elif [ $USE_SYSTEMD -eq 1 ]; then
	service hyperion stop 2>/dev/null
	#many people installed with the official script and this just uses service, if both registered -> dead
	/usr/sbin/service hyperion stop 2>/dev/null
fi

#Install dependencies for Hyperion
if [ $OS_OPENELEC -ne 1 ]; then
	echo '---> Install/Update Hyperion dependencies (This may take a while)'
	apt-get -qq update && apt-get -qq --yes install libqtcore4 libqtgui4 libqt4-network libusb-1.0-0 ca-certificates
fi

#Check, if dtparam=spi=on is in place (not for OPENELEC)
if [ $CPU_RPI -eq 1 ] && [ $OS_OPENELEC -ne 1 ]; then
	SPIOK=`grep '^\dtparam=spi=on' /boot/config.txt | wc -l`
		if [ $SPIOK -ne 1 ]; then
			echo '---> Raspberry Pi found, but SPI is not ready, we write "dtparam=spi=on" to /boot/config.txt'
			sed -i '$a dtparam=spi=on' /boot/config.txt
			REBOOTMESSAGE="echo Please reboot your Raspberry Pi, we inserted dtparam=spi=on to /boot/config.txt"
	fi
fi

#Check, if dtparam=spi=on is in place (just for OPENELEC)
if [ $CPU_RPI -eq 1 ] && [ $OS_OPENELEC -eq 1 ]; then
	SPIOK=`grep '^\dtparam=spi=on' /flash/config.txt | wc -l`
		if [ $SPIOK -ne 1 ]; then
			mount -o remount,rw /flash
			echo '---> Raspberry Pi with OpenELEC found, but SPI is not ready, we write "dtparam=spi=on" to /flash/config.txt'
			sed -i '$a dtparam=spi=on' /flash/config.txt
			mount -o remount,ro /flash
			REBOOTMESSAGE="echo Please reboot your OpenELEC, we inserted dtparam=spi=on to /flash/config.txt"
	fi
fi
#Backup the .conf files, if present
echo '---> Backup Hyperion configuration(s), if present'
rm -f /tmp/*.json 2>/dev/null
if [ $OS_OPENELEC -eq 1 ]; then
	cp -v /storage/.config/*.json /tmp 2>/dev/null
else cp -v /opt/hyperion/config/*.json /tmp 2>/dev/null
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
	OE_DEPENDECIES=$HYPERION_ADDRESS/hyperion.deps.openelec-x32x64.tar.gz
elif [ $CPU_X32 -eq 1 ]; then
	HYPERION_RELEASE=$HYPERION_ADDRESS/hyperion_x32.tar.gz
	OE_DEPENDECIES=$HYPERION_ADDRESS/hyperion.deps.openelec-x32x64.tar.gz
else
	echo "---> Critical Error: Target platform unknown -> abort"
	exit 1
fi

# Get and extract the Hyperion binaries and effects
echo '---> Downloading the appropriate Hyperion release'
if [ $OS_OPENELEC -eq 1 ]; then
	# OpenELEC has a readonly file system. Use alternative location
	echo '---> Downloading Hyperion OpenELEC release'
	curl -# -L --get $HYPERION_RELEASE | tar -C /storage -xz
	echo '---> Downloading Hyperion OpenELEC dependencies'
	curl -# -L --get $OE_DEPENDECIES | tar -C /storage/hyperion/bin -xz
	#set the executen bit (failsave)
	chmod +x -R /storage/hyperion/bin
	# modify the default config to have a correct effect path
	sed -i 's:/opt:/storage:g' /storage/hyperion/config/hyperion.config.json

	# /storage/.config is available as samba share. A symbolic link would not be working
	false | cp -i /storage/hyperion/config/hyperion.config.json /storage/.config/hyperion.config.json 2>/dev/null
else
	wget -nv $HYPERION_RELEASE -O - | tar -C /opt -xz
	#set the executen bit (failsave)
	chmod +x -R /opt/hyperion/bin
	# create links to the binaries
	ln -fs /opt/hyperion/bin/hyperiond /usr/bin/hyperiond
	ln -fs /opt/hyperion/bin/hyperion-remote /usr/bin/hyperion-remote
	ln -fs /opt/hyperion/bin/hyperion-v4l2 /usr/bin/hyperion-v4l2
	ln -fs /opt/hyperion/bin/hyperion-dispmanx /usr/bin/hyperion-dispmanx 2>/dev/null
	ln -fs /opt/hyperion/bin/hyperion-x11 /usr/bin/hyperion-x11 2>/dev/null

# Copy a link to the hyperion configuration file to /etc (-s for people who replaced the symlink with their config)
	ln -s /opt/hyperion/config/hyperion.config.json /etc/hyperion.config.json 2>/dev/null
fi
	
# Restore backup of .conf files, if present
echo '---> Restore Hyperion configuration(s), if present'
if [ $OS_OPENELEC -eq 1 ]; then
	mv -v /tmp/*.json /storage/.config/ 2>/dev/null
else mv -v /tmp/*.json /opt/hyperion/config/ 2>/dev/null	
fi

# Copy the service control configuration to /etc/int (-n to respect user modified scripts)
if [ $USE_INITCTL -eq 1 ]; then
	echo '---> Installing initctl script'
	cp -n /opt/hyperion/init.d/hyperion.initctl.sh /etc/init/hyperion.conf 2>/dev/null
	initctl reload-configuration
elif [ $OS_OPENELEC -eq 1 ]; then
	# only add to start script if hyperion is not present yet
	if [ `cat /storage/.config/autostart.sh 2>/dev/null | grep hyperiond | wc -l` -eq 0 ]; then
		echo '---> Adding Hyperion to OpenELEC autostart.sh'
		echo "/storage/hyperion/bin/hyperiond.sh /storage/.config/hyperion.config.json > /dev/null 2>&1 &" >> /storage/.config/autostart.sh
		chmod +x /storage/.config/autostart.sh
	fi
elif [ $USE_SYSTEMD -eq 1 ]; then
	echo '---> Installing systemd script'
	#place startup script for systemd and activate
	#Problem with systemd to enable symlinks - Bug? Workaround cp -n (overwrite never)
	#Bad workaround for Jessie users that used the official script for install
	update-rc.d -f hyperion remove 2>/dev/null
	rm /etc/init.d/hyperion 2>/dev/null
	cp -n /opt/hyperion/init.d/hyperion.systemd.sh /etc/systemd/system/hyperion.service
	systemctl -q enable hyperion.service
		if [ $OS_OSMC -eq 1 ]; then
			echo '---> Modify systemd script for OSMC usage'
			# Wait until kodi is sarted (for xbmc checker) and replace user (for remote control through osmc)
			sed -i '/After = mediacenter.service/d' /etc/systemd/system/hyperion.service
			sed -i '/Unit/a After = mediacenter.service' /etc/systemd/system/hyperion.service
			sed -i 's/User=root/User=osmc/g' /etc/systemd/system/hyperion.service
			sed -i 's/Group=root/Group=osmc/g' /etc/systemd/system/hyperion.service
			systemctl -q daemon-reload
		fi
elif [ $USE_SERVICE -eq 1 ]; then
	echo '---> Installing startup script in init.d'
	# place startup script in init.d and add it to upstart (-s to respect user modified scripts)
	ln -s /opt/hyperion/init.d/hyperion.init.sh /etc/init.d/hyperion 2>/dev/null
	chmod +x /etc/init.d/hyperion
	update-rc.d hyperion defaults 98 02
fi

# Start the hyperion daemon
echo '---> Starting Hyperion'
if [ $OS_OPENELEC -eq 1 ]; then
	/storage/.config/autostart.sh
elif [ $USE_INITCTL -eq 1 ]; then
	/sbin/initctl start hyperion
elif [ $USE_SERVICE -eq 1 ]; then
	/usr/sbin/service hyperion start
elif [ $USE_SYSTEMD -eq 1 ]; then
	service hyperion start
fi

#Hint for the user with path to config
if [ $OS_OPENELEC -eq 1 ];then
	HINTMESSAGE="echo Path to your configuration -> /storage/.config/hyperion.config.json"
else HINTMESSAGE="echo Path to your configuration -> /etc/hyperion.config.json"
fi
echo '*******************************************************************************' 
echo 'Hyperion Installation/Update finished!' 
echo 'Please get a new HyperCon version to benefit from the latest features!' 
echo 'Create a new config file, if you encounter problems!' 
$HINTMESSAGE
$REBOOTMESSAGE
echo '*******************************************************************************' 
## Force reboot and prevent prompt if spi is added during a HyperCon Install
if [ "$1" = "HyperConInstall" ] && [ $CPU_RPI -eq 1 ] && [ $SPIOK -ne 1 ]; then
	echo "Rebooting now, we added dtparam=spi=on to config.txt"
	reboot
	exit 0
fi
#Prompt for reboot, if spi added to config.txt
if [ $CPU_RPI -eq 1 ] && [ $SPIOK -ne 1 ];then
        while true
        do
        echo -n "---> Do you want to reboot your Raspberry Pi now? (y or n) :"
        read CONFIRM
        case $CONFIRM in
        y|Y|YES|yes|Yes) break ;;
        n|N|no|NO|No)
        echo "---> No reboot - you entered \"$CONFIRM\""
        exit
        ;;
        *) echo "-> Please enter only y or n"
        esac
        done
        echo "---> You entered \"$CONFIRM\". Rebooting now..."
        reboot
fi

exit 0