#!/bin/sh

# Script for downloading and installing the latest Hyperion release

# Make sure that the boblight daemon is no longer running
BOBLIGHT_PROCNR=$(ps -e | grep "boblight" | wc -l)
if [ $BOBLIGHT_PROCNR -eq 1 ];
then
	echo 'Found running instance of boblight. Please stop boblight via XBMC menu before installing hyperion'
	exit
fi

# Stop hyperion daemon if it is running
initctl stop hyperion

# Copy the hyperion-binaries to the /usr/bin
wget -N github.com/tvdzwan/hyperion/raw/master/deploy/hyperiond -P /usr/bin/
wget -N github.com/tvdzwan/hyperion/raw/master/deploy/hyperion-remote -P /usr/bin/

# Copy the gpio changer (gpio->spi) to the /usr/bin
wget -N github.com/tvdzwan/hyperion/raw/master/deploy/gpio2spi -P /usr/bin/

# Copy the hyperion configuration file to /etc
wget -N github.com/tvdzwan/hyperion/raw/master/config/hyperion.config.json -P /etc/

# Copy the service control configuration to /etc/int
wget -N github.com/tvdzwan/hyperion/raw/master/deploy/hyperion.conf -P /etc/init/

# Set permissions
chmod +x /usr/bin/hyperiond
chmod +x /usr/bin/hyperion-remote
chmod +x /usr/bin/gpio2spi

# Start the hyperion daemon
initctl start hyperion
