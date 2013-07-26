#!/bin/sh

# Script for removing the existing boblight library and replacing it with Hyperion

# First stop the current BobLight demon and XBMC
initctl stop xbmc
initctl stop boblight

# Install the RapsiLight library
cp libbob2hyperion.so /usr/lib/libbob2hyperion.so
chmod 755 /usr/lib/libbob2hyperion.so
cp hyperion.config.json /etc/
cp hyperion.schema.json /etc/

# Remove the existing boblight client library (make backup)
cp /usr/lib/libboblight.so.0.0.0 /usr/lib/libboblight.old
# Rename the settings file to ensure that the boblight-deamon does not start
mv /etc/bobconfig.txt /etc/bobconfig.txt.old

# Link libboblight to the new installed library
ln -s /usr/lib/libbob2hyperion.so /usr/lib/libboblight.so.0.0.0

# Restart only XBMC
initctl start xbmc
