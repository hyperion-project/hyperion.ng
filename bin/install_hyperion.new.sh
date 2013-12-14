#!/bin/sh

# Script for downloading and installing the latest Hyperion release

# Find out if we are on XBian
IS_XBIAN=`cat /etc/issue | grep XBian | wc -l`

# Make sure that the boblight daemon is no longer running
BOBLIGHT_PROCNR=$(ps -e | grep "boblight" | wc -l)
if [ $BOBLIGHT_PROCNR -eq 1 ];
then
	echo 'Found running instance of boblight. Please stop boblight via XBMC menu before installing hyperion'
	exit
fi

# Stop hyperion daemon if it is running
/sbin/initctl stop hyperion

# Get and extract the Hyperion binaries and effects to /opt
wget https://raw.github.com/tvdzwan/hyperion/master/deploy/hyperion.tar.gz -O - | tar -C /opt -xz

# create links to the binaries
ln -fs /opt/hyperion/bin/hyperiond /usr/bin/hyperiond
ln -fs /opt/hyperion/bin/hyperion-remote /usr/bin/hyperion-remote

# create link to the gpio changer (gpio->spi)
if [ $IS_XBIAN -eq 0 ]; then
	ln -fs /opt/hyperion/bin/gpio2spi /usr/bin/gpio2spi
fi

# Copy a link to the hyperion configuration file to /etc
ln -s /opt/hyperion/config/hyperion.config.json /etc/hyperion.config.json

# Copy the service control configuration to /etc/int
if [ $IS_XBIAN -eq 0 ]; then
	wget -N https://raw.github.com/tvdzwan/hyperion/master/deploy/hyperion.conf -P /etc/init/
else
	wget -N https://raw.github.com/tvdzwan/hyperion/master/deploy/hyperion.xbian.conf -O /etc/init/hyperion.conf
fi

# Start the hyperion daemon
/sbin/initctl start hyperion
