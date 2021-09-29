#!/bin/bash -e

# help print function
function printHelp {
echo "########################################################
## A script to update the user Hyperion is executed uner by the system and service manager (after start-up)
## Without arguments it will configure Hyperion being executed under the current user
##
## Homepage: https://www.hyperion-project.org
## Forum: https://forum.hyperion-project.org
########################################################
# These are possible arguments to modify the script behaviour with their default values
#
# updateHyperionUser.sh -u username   # The user name Hyperion to executed under
# updateHyperionUser.sh -h            # Shows this help message"
}

function prompt () {
	while true; do
		read -p "$1 " yn
		case "${yn:-Y}" in
			[Yes]* ) return 1;;
			[No]* ) return 0;;
			* ) echo "Please answer Yes or No.";;
		esac
	done
}

# Default username
USERNAME=${SUDO_USER}

while getopts u:h option
do
 case "${option}"
 in
 u) USERNAME=${OPTARG};;
 v) _VERBOSE=1;;    
 h) printHelp; exit 0;;
 esac
done

echo "Configure the hyperion daemon to be executed under user: ${USERNAME}"

if [ "`id -u`" -ne 0 ]; then
    echo "Please run this script as root, i.e. sudo $0"
	exit 99
fi

if ! id ${USERNAME} >/dev/null 2>&1; then
	echo "The given username does not exist. Exiting..."
	exit 99
fi

if [ ${USERNAME} == "root" ]; then
	echo ''
	echo 'You asked to run Hyperion with root privileges. This poses a security risk!'
	echo 'It is recommended not to do so unless there are good reasons (e.g. WS281x usage).'
	if prompt 'Are you sure you want to run Hyperion under root? [Yes/No]'; then
		echo 'No updates will be done. Exiting...'
		exit 99
	fi
fi

#Disable current service
CURRENT_SERVICE=`systemctl --type service | { grep -o "hyperion.*@.*\.service" || true; }`
if [[ ! -z ${CURRENT_SERVICE} ]]; then
	echo "Disable current service: ${CURRENT_SERVICE}"
	systemctl is-active --quiet ${CURRENT_SERVICE} && systemctl disable --quiet ${CURRENT_SERVICE} --now >/dev/null 2>&1
fi

HYPERION="hyperion"
#Downward compatibility
if [[ ${CURRENT_SERVICE} == hyperiond* ]]; then
	HYPERION="hyperiond"
fi

#Enable new service
NEW_SERVICE="${HYPERION}@${USERNAME}.service"
echo "Restarting Hyperion Service: ${NEW_SERVICE}"
systemctl enable --quiet ${NEW_SERVICE} --now >/dev/null 2>&1

# Update HyperBian splash screen
sed -i 's/${CURRENT_SERVICE}/${NEW_SERVICE}/' /etc/update-motd.d/10-hyperbian >/dev/null 2>&1

echo "Done."
exit 0

