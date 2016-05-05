#!/bin/sh
# Script for downloading and installing the latest Hyperion release

# Make sure /sbin is on the path (for service to find sub scripts)
PATH="/sbin:$PATH"

#Check which arguments are used
if [ "$1" = "HyperConInstall" ] || [ "$2" = "HyperConInstall" ]; then
	HCInstall=1
else HCInstall=0
fi
if [ "$1" = "BETA" ] || [ "$2" = "BETA" ]; then
	BETA=1
else BETA=0
fi

#Check, if script is running as root
if [ $(id -u) != 0 ]; then
		echo '---> Critical Error: Please run the script as root (sudo sh ./install_hyperion.sh) -> abort' 
		exit 1
fi

#Set welcome message
if [ $BETA -eq 1 ]; then
	WMESSAGE="echo This script will update Hyperion to the latest BETA"
else WMESSAGE="echo This script will install/update Hyperion Ambilight"
fi

#Welcome message
echo '*******************************************************************************' 
$WMESSAGE 
echo 'Created by brindosch - hyperion-project.org - the official Hyperion source.' 
echo '*******************************************************************************'

# Find out if we are on OpenElec (Rasplex) / OSMC / Raspbian
OS_OPENELEC=`grep -m1 -c 'OpenELEC\|RasPlex\|LibreELEC' /etc/issue`
OS_LIBREELEC=`grep -m1 -c LibreELEC /etc/issue`
OS_RASPLEX=`grep -m1 -c RasPlex /etc/issue`
OS_OSMC=`grep -m1 -c OSMC /etc/issue`
OS_RASPBIAN=`grep -m1 -c 'Raspbian\|RetroPie' /etc/issue`

# Find out which device this script runs on
CPU_RPI=`grep -m1 -c 'BCM2708\|BCM2709\|BCM2710' /proc/cpuinfo`
CPU_IMX6=`grep -m1 -c i.MX6 /proc/cpuinfo`
CPU_WETEK=`grep -m1 -c Amlogic /proc/cpuinfo`
CPU_X32X64=`uname -m | grep 'x86_32\|i686\|x86_64' | wc -l`
# Check that we have a known configuration
if [ $CPU_RPI -ne 1 ] && [ $CPU_IMX6 -ne 1 ] && [ $CPU_WETEK -ne 1 ] && [ $CPU_X32X64 -ne 1 ]; then
	echo '---> Critical Error: CPU information does not match any known releases -> abort'
	exit 1
fi

#Check which RPi we are one (in case)
RPI_1=`grep -m1 -c BCM2708 /proc/cpuinfo`
RPI_2=`grep -m1 -c BCM2709 /proc/cpuinfo`
RPI_3=`grep -m1 -c BCM2710 /proc/cpuinfo`

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
	echo '---> Critical Error: Found running instance of boblight. Please stop boblight via Kodi menu before installing hyperion -> abort'
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
			echo '---> Raspberry Pi found, but SPI is not set, we write "dtparam=spi=on" to /boot/config.txt'
			sed -i '$a dtparam=spi=on' /boot/config.txt
				if [ $HCInstall -ne 1 ]; then
				REBOOTMESSAGE="echo Please reboot your Raspberry Pi, we inserted dtparam=spi=on to /boot/config.txt"
				fi
		fi
fi

#Check, if dtparam=spi=on is in place (just for OPENELEC/LibreELEC
if [ $CPU_RPI -eq 1 ] && [ $OS_OPENELEC -eq 1 ]; then
	SPIOK=`grep '^\dtparam=spi=on' /flash/config.txt | wc -l`
		if [ $SPIOK -ne 1 ]; then
			mount -o remount,rw /flash
			echo '---> RPi with OpenELEC/LibreELEC found, but SPI is not set, we write "dtparam=spi=on" to /flash/config.txt'
			sed -i '$a dtparam=spi=on' /flash/config.txt
			mount -o remount,ro /flash
				if [ $HCInstall -ne 1 ]; then
				REBOOTMESSAGE="echo Please reboot your OpenELEC/LibreELEC, we inserted dtparam=spi=on to /flash/config.txt"
				fi
		fi
fi

#Backup the .conf files, if present
echo '---> Backup Hyperion configuration(s), if present'
rm -f /tmp/*.json 2>/dev/null
if [ $OS_OPENELEC -eq 1 ]; then
	cp -v /storage/.config/*.json /tmp 2>/dev/null
else cp -v /opt/hyperion/config/*.json /tmp 2>/dev/null
fi
 
# Select the appropriate download path
if [ $BETA -eq 1 ]; then
	HYPERION_ADDRESS=https://sourceforge.net/projects/hyperion-project/files/beta
else HYPERION_ADDRESS=https://sourceforge.net/projects/hyperion-project/files/release
fi
# Select the appropriate release
if [ $CPU_RPI -eq 1 ] && [ $OS_RASPLEX -eq 1 ]; then
	HYPERION_RELEASE=$HYPERION_ADDRESS/hyperion_rpi_rasplex.tar.gz
	OE_DEPENDECIES=$HYPERION_ADDRESS/hyperion.deps.openelec-rpi.tar.gz
elif [ $CPU_RPI -eq 1 ] && [ $OS_LIBREELEC -eq 1 ] && [ $RPI_1 -eq 1 ]; then
	HYPERION_RELEASE=$HYPERION_ADDRESS/hyperion_rpi_le.tar.gz
	OE_DEPENDECIES=$HYPERION_ADDRESS/hyperion.deps.openelec-rpi.tar.gz
elif [ $CPU_RPI -eq 1 ] && [ $OS_LIBREELEC -eq 1 ] && [ $RPI_2 -eq 1 ]; then
	HYPERION_RELEASE=$HYPERION_ADDRESS/hyperion_rpi2_le.tar.gz
	OE_DEPENDECIES=$HYPERION_ADDRESS/hyperion.deps.openelec-rpi.tar.gz
elif [ $CPU_RPI -eq 1 ] && [ $OS_LIBREELEC -eq 1 ] && [ $RPI_3 -eq 1 ]; then
	HYPERION_RELEASE=$HYPERION_ADDRESS/hyperion_rpi3_le.tar.gz
	OE_DEPENDECIES=$HYPERION_ADDRESS/hyperion.deps.openelec-rpi.tar.gz
elif [ $CPU_RPI -eq 1 ] && [ $OS_OPENELEC -eq 1 ] && [ $RPI_1 -eq 1 ]; then
	HYPERION_RELEASE=$HYPERION_ADDRESS/hyperion_rpi_oe.tar.gz
	OE_DEPENDECIES=$HYPERION_ADDRESS/hyperion.deps.openelec-rpi.tar.gz
elif [ $CPU_RPI -eq 1 ] && [ $OS_OPENELEC -eq 1 ] && [ $RPI_2 -eq 1 ]; then
	HYPERION_RELEASE=$HYPERION_ADDRESS/hyperion_rpi2_oe.tar.gz
	OE_DEPENDECIES=$HYPERION_ADDRESS/hyperion.deps.openelec-rpi.tar.gz
elif [ $CPU_RPI -eq 1 ] && [ $OS_OPENELEC -eq 1 ] && [ $RPI_3 -eq 1 ]; then
	HYPERION_RELEASE=$HYPERION_ADDRESS/hyperion_rpi3_oe.tar.gz
	OE_DEPENDECIES=$HYPERION_ADDRESS/hyperion.deps.openelec-rpi.tar.gz
elif [ $CPU_RPI -eq 1 ] && [ $OS_OSMC -eq 1 ] && [ $RPI_1 -eq 1 ]; then
	HYPERION_RELEASE=$HYPERION_ADDRESS/hyperion_rpi_osmc.tar.gz
elif [ $CPU_RPI -eq 1 ] && [ $OS_OSMC -eq 1 ] && [ $RPI_2 -eq 1 ]; then
	HYPERION_RELEASE=$HYPERION_ADDRESS/hyperion_rpi2_osmc.tar.gz
elif [ $CPU_RPI -eq 1 ] && [ $OS_OSMC -eq 1 ] && [ $RPI_3 -eq 1 ]; then
	HYPERION_RELEASE=$HYPERION_ADDRESS/hyperion_rpi3_osmc.tar.gz
elif [ $CPU_RPI -eq 1 ] && [ $RPI_1 -eq 1 ]; then
	HYPERION_RELEASE=$HYPERION_ADDRESS/hyperion_rpi.tar.gz
elif [ $CPU_RPI -eq 1 ] && [ $RPI_2 -eq 1 ]; then
	HYPERION_RELEASE=$HYPERION_ADDRESS/hyperion_rpi2.tar.gz
elif [ $CPU_RPI -eq 1 ] && [ $RPI_3 -eq 1 ]; then
	HYPERION_RELEASE=$HYPERION_ADDRESS/hyperion_rpi3.tar.gz
elif [ $CPU_IMX6 -eq 1 ]; then
	HYPERION_RELEASE=$HYPERION_ADDRESS/hyperion_imx6.tar.gz
	OE_DEPENDECIES=$HYPERION_ADDRESS/hyperion.deps.openelec-imx6.tar.gz
elif [ $CPU_WETEK -eq 1 ]; then
	HYPERION_RELEASE=$HYPERION_ADDRESS/hyperion_wetek.tar.gz
	OE_DEPENDECIES=$HYPERION_ADDRESS/hyperion.deps.openelec-rpi.tar.gz
elif [ $CPU_X32X64 -eq 1 ]; then
	HYPERION_RELEASE=$HYPERION_ADDRESS/hyperion_x86x64.tar.gz
	OE_DEPENDECIES=$HYPERION_ADDRESS/hyperion.deps.openelec-x86x64.tar.gz
else
	echo "---> Critical Error: Target platform unknown -> abort"
	exit 1
fi

# Get and extract the Hyperion binaries and effects
echo '---> Downloading the appropriate Hyperion release'
if [ $OS_OPENELEC -eq 1 ]; then
	# OpenELEC has a readonly file system. Use alternative location
	echo '---> Downloading Hyperion OpenELEC/LibreELEC release'
	curl -# -L --get $HYPERION_RELEASE | tar -C /storage -xz
	echo '---> Downloading Hyperion OpenELEC/LibreELEC dependencies'
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
	#modify all old installs with a logfile output
	sed -i 's|/dev/null|/storage/logfiles/hyperion.log|g' /storage/.config/autostart.sh 2>/dev/null
	# only add to start script if hyperion is not present yet
	mkdir /storage/logfiles 2>/dev/null
	touch /storage/.config/autostart.sh 2>/dev/null
	if [ `cat /storage/.config/autostart.sh 2>/dev/null | grep hyperiond | wc -l` -eq 0 ]; then
		echo '---> Adding Hyperion to OpenELEC/LibreELEC autostart.sh'
		echo "/storage/hyperion/bin/hyperiond.sh /storage/.config/hyperion.config.json > /storage/logfiles/hyperion.log 2>&1 &" >> /storage/.config/autostart.sh
		chmod +x /storage/.config/autostart.sh
	fi
	# only add hyperion-x11 to startup, if not found and x32x64 detected
	if [ $CPU_X32X64 -eq 1 ] && [ `cat /storage/.config/autostart.sh 2>/dev/null | grep hyperion-x11 | wc -l` -eq 0 ]; then
		echo '---> Adding Hyperion-x11 to OpenELEC/LibreELEC autostart.sh'
		echo "DISPLAY=:0.0 LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/storage/hyperion/bin /storage/hyperion/bin/hyperion-x11 </dev/null >/storage/logfiles/hyperion.log 2>&1 &" >> /storage/.config/autostart.sh		
	fi
elif [ $USE_SYSTEMD -eq 1 ]; then
	echo '---> Installing systemd script'
	#place startup script for systemd and activate
	#Problem with systemd to enable symlinks - Bug? Workaround cp -n (overwrite never)
	#Bad workaround for Jessie (systemd) users that used the official script for install
	update-rc.d -f hyperion remove 2>/dev/null
	rm /etc/init.d/hyperion 2>/dev/null
	cp -n /opt/hyperion/init.d/hyperion.systemd.sh /etc/systemd/system/hyperion.service
	systemctl -q enable hyperion.service
		if [ $OS_OSMC -eq 1 ]; then
			echo '---> Modify systemd script for OSMC usage'
			# Wait until kodi is sarted (for kodi checker)
			sed -i '/After = mediacenter.service/d' /etc/systemd/system/hyperion.service
			sed -i '/Unit/a After = mediacenter.service' /etc/systemd/system/hyperion.service
			sed -i 's/User=osmc/User=root/g' /etc/systemd/system/hyperion.service
			sed -i 's/Group=osmc/Group=root/g' /etc/systemd/system/hyperion.service
			systemctl -q daemon-reload
		fi
elif [ $USE_SERVICE -eq 1 ]; then
	echo '---> Installing startup script in init.d'
	# place startup script in init.d and add it to upstart (-s to respect user modified scripts)
	ln -s /opt/hyperion/init.d/hyperion.init.sh /etc/init.d/hyperion 2>/dev/null
	chmod +x /etc/init.d/hyperion
	update-rc.d hyperion defaults 98 02
fi

#chown the /config/ dir and all configs inside for hypercon config upload for non-root logins
if [ $OS_OSMC -eq 1 ]; then
	chown -R osmc:osmc /opt/hyperion/config
elif [ $OS_RASPBIAN -eq 1 ]; then
	chown -R pi:pi /opt/hyperion/config
fi

# Start the hyperion daemon
echo '---> Starting Hyperion'
if [ $OS_OPENELEC -eq 1 ]; then
	/storage/.config/autostart.sh > /dev/null 2>&1 &
elif [ $USE_INITCTL -eq 1 ]; then
	/sbin/initctl start hyperion
elif [ $USE_SERVICE -eq 1 ]; then
	/usr/sbin/service hyperion start
elif [ $USE_SYSTEMD -eq 1 ]; then
	service hyperion start
fi

echo '*******************************************************************************' 
echo 'Hyperion Installation/Update finished!' 
echo 'Please download the latest HyperCon version to benefit from new features!' 
echo 'To create a config, follow the HyperCon Guide at our Wiki (EN/DE)!' 
echo 'Wiki: wiki.hyperion-project.org Webpage: www.hyperion-project.org' 
$REBOOTMESSAGE
echo '*******************************************************************************' 
## Force reboot and prevent prompt if spi is added during a HyperCon Install
if [ $HCInstall -eq 1 ] && [ $CPU_RPI -eq 1 ] && [ $SPIOK -ne 1 ]; then
	echo "Rebooting now, we added dtparam=spi=on to config.txt"
	reboot
	exit 0
fi
#Prompt for reboot, if spi added to config.txt
if [ $CPU_RPI -eq 1 ] && [ $SPIOK -ne 1 ]; then
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