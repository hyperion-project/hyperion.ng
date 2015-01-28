#!/bin/sh

# Make sure /sbin is on the path (for service to find sub scripts)
PATH="/sbin:$PATH"

# Script for downloading and installing the latest Hyperion release

# Find out if we are on Raspbmc
IS_XBIAN=`cat /etc/issue | grep XBian | wc -l`
IS_RASPBMC=`cat /etc/issue | grep Raspbmc | wc -l`
IS_OPENELEC=`cat /etc/issue | grep -m 1 OpenELEC | wc -l`

# Find out if its an imx6 device
IS_IMX6=`cat /proc/cpuinfo | grep i.MX6 | wc -l`

# check which init script we should use
USE_INITCTL=`which /sbin/initctl | wc -l`
USE_SERVICE=`which /usr/sbin/service | wc -l`

# Make sure that the boblight daemon is no longer running
BOBLIGHT_PROCNR=$(pidof boblightd | wc -l)
if [ $BOBLIGHT_PROCNR -eq 1 ];
then
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

# Get and extract the Hyperion binaries and effects
echo 'Downloading hyperion'
if [ $IS_OPENELEC -eq 1 ]; then
	# OpenELEC has a readonly file system. Use alternative location
if [ $IS_IMX6 -eq 1 ]; then
	curl -L --get https://raw.githubusercontent.com/tvdzwan/hyperion/master/deploy/hyperion_imx6.tar.gz | tar -C /storage -xz
else
	curl -L --get https://raw.githubusercontent.com/tvdzwan/hyperion/master/deploy/hyperion.tar.gz | tar -C /storage -xz
fi
	curl -L --get https://raw.githubusercontent.com/tvdzwan/hyperion/master/deploy/hyperion.deps.openelec-rpi.tar.gz | tar -C /storage/hyperion/bin -xz
	# modify the default config to have a correct effect path
	sed -i 's:/opt:/storage:g' /storage/hyperion/config/hyperion.config.json
else
if [ $IS_IMX6 -eq 1 ]; then
	wget https://raw.githubusercontent.com/tvdzwan/hyperion/master/deploy/hyperion_imx6.tar.gz -O - | tar -C /opt -xz
else
	wget https://raw.githubusercontent.com/tvdzwan/hyperion/master/deploy/hyperion.tar.gz -O - | tar -C /opt -xz
fi
fi

# create links to the binaries
if [ $IS_OPENELEC -ne 1 ]; then
	ln -fs /opt/hyperion/bin/hyperiond /usr/bin/hyperiond
	ln -fs /opt/hyperion/bin/hyperion-remote /usr/bin/hyperion-remote
	ln -fs /opt/hyperion/bin/hyperion-v4l2 /usr/bin/hyperion-v4l2
fi

# create link to the gpio changer (gpio->spi)
if [ $IS_RASPBMC -eq 1 ] && [ $IS_IMX6 -ne 1 ]; then
	ln -fs /opt/hyperion/bin/gpio2spi /usr/bin/gpio2spi
fi

# Copy a link to the hyperion configuration file to /etc
if [ $IS_OPENELEC -eq 1 ]; then
	# copy to alternate location, because of readonly file system
	# /storage/.config is available as samba share. A symbolic link would not be working
	false | cp -i /storage/hyperion/config/hyperion.config.json /storage/.config/hyperion.config.json 2>/dev/null
else
	ln -s /opt/hyperion/config/hyperion.config.json /etc/hyperion.config.json
fi

# Copy the service control configuration to /etc/int
if [ $USE_INITCTL -eq 1 ]; then
	echo 'Installing initctl script'
	if [ $IS_RASPBMC -eq 1 ]; then
		wget -N https://raw.githubusercontent.com/tvdzwan/hyperion/master/deploy/hyperion.conf -P /etc/init/
	else
		wget -N https://raw.githubusercontent.com/tvdzwan/hyperion/master/deploy/hyperion.xbian.conf -O /etc/init/hyperion.conf
	fi
elif [ $USE_SERVICE -eq 1 ]; then
	echo 'Installing startup script in init.d'
	# place startup script in init.d and add it to upstart
	ln -fs /opt/hyperion/init.d/hyperion.init.sh /etc/init.d/hyperion
	chmod +x /etc/init.d/hyperion
	update-rc.d hyperion defaults 98 02
elif [ $IS_OPENELEC -eq 1 ]; then
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
