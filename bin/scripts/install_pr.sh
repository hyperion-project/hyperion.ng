#!/bin/bash
# Script for downloading a specific open Pull Request Artifact from Hyperion.NG

# Fixed variables
api_url="https://api.github.com/repos/hyperion-project/hyperion.ng"
type wget > /dev/null 2> /dev/null
hasWget=$?
type curl > /dev/null 2> /dev/null
hasCurl=$?
type python3 > /dev/null 2> /dev/null
hasPython3=$?
type python > /dev/null 2> /dev/null
hasPython2=$?

DISTRIBUTION="debian"
CODENAME="bullseye"
ARCHITECTURE=""
WITH_QT5=false

BASE_PATH='.';

if [[ "${hasWget}" -ne 0 ]] && [[ "${hasCurl}" -ne 0 ]]; then
	echo '---> Critical Error: wget or curl required to download pull request artifacts'
	exit 1
fi

if [[ "${hasPython3}" -eq 0 ]]; then
    pythonCmd="python3"
else
    if [[ "${hasPython2}" -eq 0 ]]; then
        pythonCmd="python"
    else
	    echo '---> Critical Error: python3 or python2 required to download pull request artifacts'
	fi
	exit 1
fi

function request_call() {
	if [ $hasWget -eq 0 ]; then
		echo $(wget --quiet --header="Authorization: token ${PR_TOKEN}" -O - $1)
	elif [ $hasCurl -eq 0 ]; then
		echo $(curl -skH "Authorization: token ${PR_TOKEN}" $1)
	fi
}

while getopts ":a:c:r:t:5" opt; do
  case "$opt" in
    a) ARCHITECTURE=$OPTARG ;;
    c) CONFIGDIR=$OPTARG ;;
    r) run_id=$OPTARG ;;
    t) PR_TOKEN=$OPTARG ;;
    5) WITH_QT5=true ;;    
  esac
done
shift $(( OPTIND - 1 ))

# Check for a command line argument (PR number)
if [ "$1" == "" ] || [ $# -gt 1 ] || [ -z ${PR_TOKEN} ]; then
	echo "Usage: $0 -t <git_token> -a <architecture> -r <run_id> -c <hyperion config directory> <PR_NUMBER>" >&2
	exit 1
else
	pr_number="$1"
fi

# Set welcome message
echo '*******************************************************************************'
echo 'This script will download a specific open Pull Request Artifact from Hyperion.NG'
echo 'Created by hyperion-project.org - the official Hyperion source.'
echo '*******************************************************************************'

# Determine the architecture, if not given
if [[ -z ${ARCHITECTURE} ]]; then
	ARCHITECTURE=`uname -m`
fi

#Test if multiarchitecture setup, i.e. user-space is 32bit
if [[ "${ARCHITECTURE}" == "aarch64" || "${ARCHITECTURE}" == "arm64" ]]; then
	ARCHITECTURE="arm64"
	USER_ARCHITECTURE=$ARCHITECTURE
	IS_V7L=`cat /proc/$$/maps |grep -m1 -c v7l`
	if [ $IS_V7L -ne 0 ]; then
		USER_ARCHITECTURE="armv7"
	else
	   IS_V6L=`cat /proc/$$/maps |grep -m1 -c v6l`
	   if [ $IS_V6L -ne 0 ]; then
		   USER_ARCHITECTURE="armv6"
	   fi
	fi
    if [ $ARCHITECTURE != $USER_ARCHITECTURE ]; then
        echo "---> Identified user space target architecture: $USER_ARCHITECTURE"
        ARCHITECTURE=$USER_ARCHITECTURE
    fi
else
	ARCHITECTURE=${ARCHITECTURE//x86_/amd}
fi

echo 'armv6l armv7l arm64 amd64' | grep -qw ${ARCHITECTURE}
if [ $? -ne 0 ]; then
    echo "---> Critical Error: Target architecture $ARCHITECTURE is unknown -> abort"
    exit 1
else
	PACKAGE="${ARCHITECTURE}"
	QTVERSION="5"
	if [ ${WITH_QT5} == false ]; then
		QTVERSION="6"
		PACKAGE="${PACKAGE}_qt6"
	fi

    echo "---> Download package for identified runtime architecture: $ARCHITECTURE and Qt$QTVERSION"
fi

# Determine if PR number exists
pulls=$(request_call "$api_url/pulls?state=open")

pr_exists=$(echo "$pulls" | tr '\r\n' ' ' | ${pythonCmd} -c """
import json,sys
data = json.load(sys.stdin)

for i in data:
	if i['number'] == "$pr_number":
		print('exists')
		break
""" 2>/dev/null)

if [ "$pr_exists" != "exists" ]; then
	echo "---> Pull Request $pr_number not found as open PR -> abort"
	exit 1
fi

# Get head_sha value from 'pr_number'
head_sha=$(echo "$pulls" | tr '\r\n' ' ' | ${pythonCmd} -c """
import json,sys
data = json.load(sys.stdin)

for i in data:
	if i['number'] == "$pr_number":
		print(i['head']['sha'])
		break
""" 2>/dev/null)

if [ -z "$head_sha" ]; then
	echo "---> The specified PR #$pr_number has no longer any artifacts or has been closed."
	echo "---> It may be older than 14 days or a new build currently in progress. Ask the PR creator to recreate the artifacts at the following URL:"
	echo "---> https://github.com/hyperion-project/hyperion.ng/pull/$pr_number"
	exit 1
fi

if [ -z "$run_id" ]; then
# Determine run_id from head_sha
runs=$(request_call "$api_url/actions/runs?head_sha=$head_sha")
run_id=$(echo "$runs" | tr '\r\n' ' ' | ${pythonCmd} -c """
import json,sys,os
data = json.load(sys.stdin)

for i in data['workflow_runs']:
	if os.path.basename(i['path']) == 'push_pull.yml':
		print(i['id'])
		break
""" 2>/dev/null)
fi

if [ -z "$run_id" ]; then
	echo "---> The specified PR #$pr_number has no longer any artifacts."
	echo "---> It may be older than 14 days. Ask the PR creator to recreate the artifacts at the following URL:"
	echo "---> https://github.com/hyperion-project/hyperion.ng/pull/$pr_number"
	exit 1
fi

# Get archive_download_url from workflow
artifacts=$(request_call "$api_url/actions/runs/$run_id/artifacts")

PACKAGE_NAME=$(echo "$artifacts" | tr '\r\n' ' ' | ${pythonCmd} -c """
import json,sys, re
data = json.load(sys.stdin)

for i in data['artifacts']:
     if re.match('.*{}$'.format(re.escape('$PACKAGE')), i['name']):
        print(i['name'])
        break
""" 2>/dev/null)

archive_download_url=$(echo "$artifacts" | tr '\r\n' ' ' | ${pythonCmd} -c """
import json,sys, re
data = json.load(sys.stdin)

for i in data['artifacts']:
    if re.match('.*{}$'.format(re.escape('$PACKAGE')), i['name']):
        print(i['archive_download_url'])
        break
""" 2>/dev/null)

if [ -z "$archive_download_url" ]; then
	echo "---> The specified PR #$pr_number has no longer any artifacts."
	echo "---> It may be older than 14 days. Ask the PR creator to recreate the artifacts at the following URL:"
	echo "---> https://github.com/hyperion-project/hyperion.ng/pull/$pr_number"
	exit 1
fi

# Download packed PR artifact
echo "---> Downloading Pull Request #$pr_number, package: $PACKAGE_NAME"
if [ $hasCurl -eq 0 ]; then
	curl -skH "Authorization: token ${PR_TOKEN}" -o $BASE_PATH/temp.zip -L --get $archive_download_url
elif [ $hasWget -eq 0 ]; then
    echo "wget"
	wget --quiet --header="Authorization: token ${PR_TOKEN}" -O $BASE_PATH/temp.zip $archive_download_url
fi

# Create new folder & extract PR artifact
echo "---> Extracting packed Artifact"
mkdir -p $BASE_PATH/hyperion_pr$pr_number
unzip -p $BASE_PATH/temp.zip | tar --strip-components=2 -C $BASE_PATH/hyperion_pr$pr_number share/hyperion/ -xz

# Delete PR artifact
echo '---> Remove temporary files'
rm $BASE_PATH/temp.zip 2>/dev/null

# Create the startup script
echo '---> Create startup script'
STARTUP_SCRIPT="#!/bin/bash -e

# Stop hyperion service, if it is running
"'CURRENT_SERVICE=$(systemctl --type service | { grep -o "hyperion.*\.service" || true; })
if [[ ! -z ${CURRENT_SERVICE} ]]; then
    echo "---> Stop current service: ${CURRENT_SERVICE}"

    STOPCMD="systemctl stop --quiet ${CURRENT_SERVICE} --now"
    USERNAME=${SUDO_USER:-$(whoami)}
    if [ ${USERNAME} != "root" ]; then
        STOPCMD="sudo ${STOPCMD}"
    fi

    ${STOPCMD} >/dev/null 2>&1
    if [ $? -ne 0 ]; then
       echo "---> Critical Error: Failed to stop service: ${CURRENT_SERVICE}, Hyperion may not be started. Stop Hyperion manually."
    else
       echo "---> Service ${CURRENT_SERVICE} successfully stopped, Hyperion will be started"
    fi
fi'""

TARGET_CONFIGDIR="$BASE_PATH/config"

if [[ ! -z ${CONFIGDIR} ]]; then
STARTUP_SCRIPT+="
# Copy existing configuration file
"'echo "Copy existing configuration from "'${CONFIGDIR}"
mkdir -p "$TARGET_CONFIGDIR"
cp -ri "${CONFIGDIR}/*" "$TARGET_CONFIGDIR""
fi

STARTUP_SCRIPT+="
# Start PR artifact
cd $BASE_PATH/hyperion_pr$pr_number
./bin/hyperiond -d -u $TARGET_CONFIGDIR"

# Place startup script
echo "$STARTUP_SCRIPT" > $BASE_PATH/hyperion_pr$pr_number/$pr_number.sh

# Set the executen bit
chmod +x -R $BASE_PATH/hyperion_pr$pr_number/$pr_number.sh

echo "*******************************************************************************"
echo "Download finished!"
$REBOOTMESSAGE
echo "You can test the pull request with this command: $BASE_PATH/hyperion_pr$pr_number/$pr_number.sh"
echo "Remove the test installation with: rm -R $BASE_PATH/hyperion_pr$pr_number"
echo "Feedback is welcome at https://github.com/hyperion-project/hyperion.ng/pull/$pr_number"
echo "*******************************************************************************"
