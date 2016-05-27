#!/bin/bash
# Script to add a second or more hyperion instance(s) to the corresponding system service

# Make sure /sbin is on the path (for service to find sub scripts)
PATH="/sbin:$PATH"

#Check, if script is running as root
if [ $(id -u) != 0 ]; then
		echo '---> Critical Error: Please run the script as root (sudo sh ./setup_hyperion_forward.sh) -> abort' 
		exit 1
fi

#Welcome message
echo '*******************************************************************************' 
echo 'This setup script will duplicate the hyperion service'
echo 'Choose the name(s) for one or more config files - one service for each config' 
echo 'Created by brindosch - hyperion-project.org - the official Hyperion source.' 
echo '*******************************************************************************'

#Prompt for confirmation to proceed
while true
do
echo -n "---> Do you really want to proceed? (y or n) :"
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
echo "---> You entered \"$CONFIRM\". We will proceed!"
echo ""

#Check which system we are on
OS_OPENELEC=`grep -m1 -c 'OpenELEC\|RasPlex\|LibreELEC' /etc/issue`
USE_SYSTEMD=`grep -m1 -c systemd /proc/1/comm`
USE_INITCTL=`which /sbin/initctl | wc -l`
USE_SERVICE=`which /usr/sbin/service | wc -l`

#Setting up the paths to service files
if [ $USE_INITCTL -eq 1 ]; then
	SERVICEPATH=/etc/init
elif [ $OS_OPENELEC -eq 1 ]; then
	SERVICEPATH=/storage/.config
elif [ $USE_SYSTEMD -eq 1 ]; then
	SERVICEPATH=/etc/systemd/system
elif [ $USE_SERVICE -eq 1 ]; then
	SERVICEPATH/etc/init.d
fi

#Setting up the default PROTO/JSON ports
JSONPORT=19444
PROTOPORT=19445
# and service count
SERVICEC=1

#Setting up the paths to config files
if [ $OS_OPENELEC -eq 1 ]; then
	CONFIGPATH=/storage/.config
else CONFIGPATH=/opt/hyperion/config
fi

#Ask the user for some informations regarding the setup
echo "---> Please enter the config name(s) you want to create"
echo "---> Information: One name creates one service and two names two services etc"
echo '---> Please enter them seperated with a space in a one line row!'
echo '---> example: hyperion.philipshue_1.json hyperion.AtmoOrb_2.json hypthreeconf.json'
echo '---> In any case, add ".json" at the end of each file name'
read -p 'Config file name(s): ' FILENAMES
echo '---> Thank you, we will modify your Hyperion installation now'
sleep 2

#Processing input
set $FILENAMES
FWCOUNT=${#}

#Convert all old config file paths to make sure this script is working (default for new installs with 1.02.0 and higher)
if [ $USE_INITCTL -eq 1 ]; then
	sed -i "s|/etc/hyperion.config.json|/etc/hyperion/hyperion.config.json|g" $SERVICEPATH/hyperion.conf
elif [ $OS_OPENELEC -eq 1 ]; then
	sleep 0
elif [ $USE_SYSTEMD -eq 1 ]; then
	sed -i "s|/etc/hyperion.config.json|/etc/hyperion/hyperion.config.json|g" $SERVICEPATH/hyperion.service
elif [ $USE_SERVICE -eq 1 ]; then
	sed -i "s|/etc/hyperion.config.json|/etc/hyperion/hyperion.config.json|g" $SERVICEPATH/hyperion
fi

#Processing service files
if [ $USE_INITCTL -eq 1 ]; then
	echo "---> Initctl detected, processing service files"
	while [ $SERVICEC -le $FWCOUNT ]; do
	echo "Processing service ${SERVICEC}: \"hyperion_fw${SERVICEC}.conf\""
		if [ -e "${SERVICEPATH}/hyperion_fw${SERVICEC}.conf" ]; then
			echo "Service was already created - skipped"
			echo "Input \"${1}\" was skipped"
		else	
			echo "Create ${SERVICEPATH}/hyperion_fw${SERVICEC}.conf"
			cp -s $SERVICEPATH/hyperion.conf $SERVICEPATH/hyperion_fw$SERVICEC.conf
			echo "Config name changed to \"${1}\" inside \"hyperion_fw${SERVICEC}.conf\""
			sed -i "s/hyperion.config.json/$1/g" $SERVICEPATH/hyperion_fw$SERVICEC.conf
			initctl reload-configuration
		fi
	shift
	SERVICEC=$((SERVICEC + 1))
	done
elif [ $OS_OPENELEC -eq 1 ]; then
	echo "---> OE/LE detected, processing autostart.sh"
	while [ $SERVICEC -le $FWCOUNT ]; do
	echo "${SERVICEC}. processing OE autostart.sh entry \"${1}\""
		OE=`grep -m1 -c ${1} $SERVICEPATH/autostart.sh`
		if [ $OE -eq 0 ]; then
			echo "Add config name \"${1}\" to \"autostart.sh\""
			echo "/storage/hyperion/bin/hyperiond.sh /storage/.config/${1} > /storage/logfiles/hyperion_fw${SERVICEC}.log 2>&1 &" >> /storage/.config/autostart.sh
		else
			echo "\"${1}\" was already added - skipped"
		fi
	shift
	SERVICEC=$((SERVICEC + 1))
	done	
elif [ $USE_SYSTEMD -eq 1 ]; then
	echo "---> Systemd detected, processing service files"
	while [ $SERVICEC -le $FWCOUNT ]; do
	echo "Processing service ${SERVICEC}: \"hyperion_fw${SERVICEC}.service\""
		if [ -e "${SERVICEPATH}/hyperion_fw${SERVICEC}.service" ]; then
			echo "Service was already created - skipped"
			echo "Input \"${1}\" was skipped"
		else
			echo "Create ${SERVICEPATH}/hyperion_fw${SERVICEC}.service"
			cp -s $SERVICEPATH/hyperion.service $SERVICEPATH/hyperion_fw$SERVICEC.service
			echo "Config name changed to \"${1}\" inside \"hyperion_fw${SERVICEC}.service\""
			sed -i "s/hyperion.config.json/$1/g" $SERVICEPATH/hyperion_fw$SERVICEC.service
			systemctl -q enable hyperion_fw$SERVICEC.service
		fi
	shift
	SERVICEC=$((SERVICEC + 1))
	done
elif [ $USE_SERVICE -eq 1 ]; then
	echo "---> Init.d detected, processing service files"
	while [ $SERVICEC -le $FWCOUNT ]; do
	echo "Processing service ${SERVICEC}: \"hyperion_fw${SERVICEC}\""
		if [ -e "${SERVICEPATH}/hyperion_fw${SERVICEC}" ]; then
			echo "Service was already created - skipped"
			echo "Input \"${1}\" was skipped"
		else
			echo "Create ${SERVICEPATH}/hyperion_fw${SERVICEC}"
			cp -s $SERVICEPATH/hyperion $SERVICEPATH/hyperion_fw$SERVICEC
			echo "Config name changed to \"${1}\" inside \"hyperion_fw${SERVICEC}\""
			sed -i "s/hyperion.config.json/$1/g" $SERVICEPATH/hyperion_fw$SERVICEC
			update-rc.d hyperion_fw$SERVICEC defaults 98 02
		fi
	shift
	SERVICEC=$((SERVICEC + 1))
	done	
fi

#Service creation done
echo '*******************************************************************************'
echo 'Script done all actions - all input processed'
echo 'Now upload your configuration(s) with HyperCon at the SSH Tab'
echo 'All created Hyperion services will start with your chosen confignames'
echo 'Wiki: wiki.hyperion-project.org Webpage: www.hyperion-project.org' 
echo '*******************************************************************************'