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
wget github.com/tvdzwan/hyperion/tree/master/deploy/hyperiond -P /usr/bin/
wget github.com/tvdzwan/hyperion/tree/master/deploy/hyperion-remote -P /usr/bin/

# Copy the hyperion configuration file to /etc
wget github.com/tvdzwan/hyperion/tree/master/config/hyperion.config.json -P /etc/

# Copy the service control configuration to /etc/int
wget github.com/tvdzwan/hyperion/tree/master/bin/hyperion.conf -P /etc/init/

# Start the hyperion daemon
initctl start hyperion
