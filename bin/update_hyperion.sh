#!/bin/sh

# Script for downloading and installing the latest Hyperion release

# Stop hyperion daemon if it is running
initctl stop hyperion

wget -N github.com/tvdzwan/hyperion/raw/master/deploy/hyperiond -P /usr/bin/
wget -N github.com/tvdzwan/hyperion/raw/master/deploy/hyperion-remote -P /usr/bin/

# Copy the gpio changer (gpio->spi) to the /usr/bin
wget -N github.com/tvdzwan/hyperion/raw/master/deploy/gpio2spi -P /usr/bin/

# Copy the service control configuration to /etc/int
wget -N github.com/tvdzwan/hyperion/raw/master/deploy/hyperion.conf -P /etc/init/

# Set permissions
chmod +x /usr/bin/hyperiond
chmod +x /usr/bin/hyperion-remote
chmod +x /usr/bin/gpio2spi

# Start the hyperion daemon
initctl start hyperion
