#!/bin/sh
# Script to remove Hyperion and all services

# Make sure /sbin is on the path (for service to find sub scripts)
PATH="/sbin:$PATH"

#Check if HyperCon is logged in as root
if [ $(id -u) != 0 ] && [ "$1" = "HyperConRemove" ]; then
		echo '---> Critical Error: Please connect as user "root" through HyperCon' 
		echo '---> We need admin privileges to remove your Hyperion! -> abort'
		exit 1
fi

#Check, if script is running as root
if [ $(id -u) != 0 ]; then
		echo '---> Critical Error: Please run the script as root (sudo sh ./remove_hyperion.sh)' 
		exit 1
fi

#Welcome message
echo '*******************************************************************************' 
echo 'This script will remove Hyperion and it´s services' 
echo '-----> Please BACKUP your hyperion.config.json if necessary <-----'
echo 'Created by brindosch - hyperion-project.org - the official Hyperion source.' 
echo '*******************************************************************************'

#Skip the prompt if HyperCon Remove
if [ "$1" = "" ]; then
#Prompt for confirmation to proceed
while true
do
echo -n "---> Do you really want to remove Hyperion and it´s services? (y or n) :"
read CONFIRM
case $CONFIRM in
y|Y|YES|yes|Yes) break ;;
n|N|no|NO|No)
echo "---> Aborting - you entered \"$CONFIRM\""
exit
;;
*) echo "-> Please enter only y or n"
esac
done
echo "---> You entered \"$CONFIRM\". Remove Hyperion!"
fi
# Find out if we are on OpenElec or RasPlex
OS_OPENELEC=`grep -m1 -c 'OpenELEC\|RasPlex\|LibreELEC' /etc/issue`

# check which init script we should use
USE_SYSTEMD=`grep -m1 -c systemd /proc/1/comm`
USE_INITCTL=`which /sbin/initctl | wc -l`
USE_SERVICE=`which /usr/sbin/service | wc -l`

# set count for forwarder
SERVICEC=1

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
#		while [ $SERVICEC -le 20 ]; do
#		service hyperion_fw$SERVICEC stop 2>/dev/null
#		((SERVICEC++))
#		done
fi

#reset count
SERVICEC=`which /usr/sbin/service | wc -l`

#Disabling and delete service files
if [ $USE_INITCTL -eq 1 ]; then
	echo '---> Delete and disable Hyperion initctl script'
	rm -v /etc/init/hyperion* 2>/dev/null
	initctl reload-configuration
elif [ $OS_OPENELEC -eq 1 ]; then
	# Remove Hyperion from OpenELEC autostart.sh
	echo "---> Remove Hyperion from OpenELEC autostart.sh"
	sed -i "/hyperiond/d" /storage/.config/autostart.sh 2>/dev/null
	sed -i "/hyperion-x11/d" /storage/.config/autostart.sh 2>/dev/null
elif [ $USE_SYSTEMD -eq 1 ]; then
	# Delete and disable Hyperion systemd script
	echo '---> Delete and disable Hyperion systemd script'
	systemctl disable hyperion.service
#		while [ $SERVICEC -le 20 ]; do
#		systemctl -q disable hyperion_fw$SERVICEC.service 2>/dev/null
#		((SERVICEC++))
#		done
	rm -v /etc/systemd/system/hyperion* 2>/dev/null
elif [ $USE_SERVICE -eq 1 ]; then
	# Delete and disable Hyperion init.d script
	echo '---> Delete and disable Hyperion init.d script'
	update-rc.d -f hyperion remove
#		while [ $SERVICEC -le 20 ]; do
#		update-rc.d -f hyperion_fw$SERVICEC remove 2>/dev/null
#		((SERVICEC++))
#		done
	rm /etc/init.d/hyperion* 2>/dev/null
fi

# Delete Hyperion binaries
if [ $OS_OPENELEC -eq 1 ]; then
	# Remove OpenELEC Hyperion binaries and configs
	echo '---> Remove the OpenELEC Hyperion binaries and hyperion.config.json'
	rm -rv /storage/hyperion 2>/dev/null
	rm -v /storage/.config/hyperion.config.json 2>/dev/null
else 	
	#Remove binaries on all distributions/systems (not OpenELEC)
	echo "---> Remove links to the binaries"	
	rm -v /usr/bin/hyperiond 2>/dev/null
	rm -v /usr/bin/hyperion-remote 2>/dev/null
	rm -v /usr/bin/hyperion-v4l2 2>/dev/null
	rm -v /usr/bin/hyperion-dispmanx 2>/dev/null
	rm -v /usr/bin/hyperion-x11 2>/dev/null
	rm -v /etc/hyperion.config.json 2>/dev/null
	echo "---> Remove binaries"
	rm -rv /opt/hyperion 2>/dev/null
fi
echo '*******************************************************************************' 
echo 'Hyperion successful removed!'
echo '*******************************************************************************'  
exit 0
	