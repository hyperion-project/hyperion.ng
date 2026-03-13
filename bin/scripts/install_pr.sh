#!/bin/bash
# Script for downloading a specific open Pull Request Artifact from Hyperion.NG

# Fixed variables
api_url="https://api.github.com/repos"
type wget >/dev/null 2>/dev/null
hasWget=$?
type curl >/dev/null 2>/dev/null
hasCurl=$?
type python3 >/dev/null 2>/dev/null
hasPython3=$?
type python >/dev/null 2>/dev/null
hasPython2=$?

REPOSITORY="hyperion-project/hyperion.ng"
DISTRIBUTION="debian"
CODEBASE=""
ARCHITECTURE=""

BASE_PATH='.'

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

request_call() {
	local url="$1"
	local response body status_code

	if [ $hasWget -eq 0 ]; then
		# Use a temp file to store headers
		local headers_file=$(mktemp)
		body=$(wget --quiet --server-response --header="Authorization: token ${PR_TOKEN}" -O - "$url" 2> "$headers_file")
		status_code=$(awk '/^  HTTP/{code=$2} END{print code}' "$headers_file")
		rm -f "$headers_file"

	elif [ $hasCurl -eq 0 ]; then
		# Append status code at the end of the response
		response=$(curl -sk -w "\n%{http_code}" -H "Authorization: token ${PR_TOKEN}" "$url")
		body=$(echo "$response" | sed '$d')                      # All but last line
		status_code=$(echo "$response" | tail -n1)               # Last line = status code
	else
		echo "---> Neither wget nor curl is available." >&2
		exit 1
	fi

	# Handle common HTTP errors
	case "$status_code" in
		401)
			echo "---> Error: 401 Unauthorized. Check your token." >&2
			exit 1
			;;
		403)
			echo "---> Error: 403 Forbidden. You might be rate-limited or lack permissions." >&2
			exit 1
			;;
		404)
			echo "---> Error: 404 Not Found. URL is incorrect or resource doesn't exist." >&2
			exit 1
			;;
		5*)
			echo "---> Error: Server error ($status_code). Try again later." >&2
			exit 1
			;;
	esac

	# Success: print response body
	echo "$body"
}

while getopts ":a:b:c:g:r:t:" opt; do
	case "$opt" in
	a) ARCHITECTURE=$OPTARG ;;
	b) CODEBASE=$OPTARG ;;
	c) CONFIGDIR=$OPTARG ;;
	g) REPOSITORY=$OPTARG ;;
	r) run_id=$OPTARG ;;
	t) PR_TOKEN=$OPTARG ;;
	esac
done
shift $((OPTIND - 1))

# Check for a command line argument (PR number)
if [ "$1" == "" ] || [ $# -gt 1 ] || [ -z ${PR_TOKEN} ]; then
	echo "Usage: $0 -t <git_token> -a <architecture> -b <codebase> -r <run_id> -c <hyperion config directory> -g <github project/repository> <PR_NUMBER>" >&2
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
	ARCHITECTURE=$(uname -m)
fi

#Test if multiarchitecture setup, i.e. user-space is 32bit
if [[ "${ARCHITECTURE}" == "aarch64" ]]; then
	ARCHITECTURE="arm64"
	USER_ARCHITECTURE=$ARCHITECTURE
	IS_ARMHF=$(grep -m1 -c armhf /proc/$$/maps)
	if [ $IS_ARMHF -ne 0 ]; then
		USER_ARCHITECTURE="armv7"
	else
		IS_ARMEL=$(grep -m1 -c armel /proc/$$/maps)
		if [ $IS_ARMEL -ne 0 ]; then
			USER_ARCHITECTURE="armv6"
		fi
	fi
	if [ $ARCHITECTURE != $USER_ARCHITECTURE ]; then
		echo "---> Identified user space target architecture: $USER_ARCHITECTURE"
		ARCHITECTURE=$USER_ARCHITECTURE
	fi
else
	# Change x86_xx to amdxx
	ARCHITECTURE=${ARCHITECTURE//x86_/amd}
	# Remove 'l' from armv6l, armv7l
	ARCHITECTURE=${ARCHITECTURE//l/}
fi

echo 'armv6 armv7 arm64 amd64' | grep -qw ${ARCHITECTURE}
if [ $? -ne 0 ]; then
	echo "---> Critical Error: Target architecture $ARCHITECTURE is unknown -> abort"
	exit 1
else
	if [[ -z ${CODEBASE} ]]; then
		PACKAGE="${ARCHITECTURE}"
		echo "---> Download package for identified runtime architecture: $ARCHITECTURE"
	else
		PACKAGE="${CODEBASE}_${ARCHITECTURE}"
		echo "---> Download package for identified runtime architecture: $ARCHITECTURE and selected codebase: $CODEBASE"
	fi
fi

api_url="${api_url}/${REPOSITORY}"


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
	if [ -z "$run_id" ]; then
		echo "---> Pull Request $pr_number not found as open PR -> abort"
		exit 1
	else
		echo "---> Pull Request $pr_number not found as open PR -> use run_id: ${run_id}"
	fi
else
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

		if [ -z "$run_id" ]; then
			echo "---> The specified PR #$pr_number has no longer any artifacts or has been closed."
			echo "---> It may be older than 14 days or a new build currently in progress. Ask the PR creator to recreate the artifacts at the following URL:"
			echo "---> https://github.com/hyperion-project/hyperion.ng/pull/$pr_number"
		  	exit 1
		fi
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
     if re.match('^(?!macOS|windows).*{}$'.format(re.escape('$PACKAGE')), i['name'], re.IGNORECASE):
        print(i['name'])
        break
""" 2>/dev/null)

archive_download_url=$(echo "$artifacts" | tr '\r\n' ' ' | ${pythonCmd} -c """
import json,sys, re
data = json.load(sys.stdin)

for i in data['artifacts']:
    if re.match('^(?!macOS|windows).*{}$'.format(re.escape('$PACKAGE')), i['name'], re.IGNORECASE):
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
echo "---> Downloading Pull Request #$pr_number, run_id: $run_id, package: $PACKAGE_NAME"
if [ $hasCurl -eq 0 ]; then
	curl -# -kH "Authorization: token ${PR_TOKEN}" -o $BASE_PATH/temp.zip -L --get $archive_download_url
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
STARTUP_SCRIPT_STOPSRVS=$(
	cat <<'EOF'
#!/bin/bash -e

# Stop hyperion service, if it is running
CURRENT_SERVICE=$(systemctl --type service | grep -o 'hyperion.*\.service' || true)

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
fi
EOF
)

TARGET_CONFIGDIR="$BASE_PATH/config"

if [[ ! -z ${CONFIGDIR} ]]; then
	STARTUP_SCRIPT_COPYCONFIG="$(
		cat <<EOF
  
# Copy existing configuration file
echo "Copy existing configuration from ${CONFIGDIR}"
mkdir -p "$TARGET_CONFIGDIR"
cp -ri "${CONFIGDIR}"/* "$TARGET_CONFIGDIR"
EOF
	)"
fi

STARTUP_SCRIPT_STARTPR="$(
	cat <<EOF

# Start PR artifact 
cd "$BASE_PATH/hyperion_pr$pr_number"
./bin/hyperiond -d -u "$TARGET_CONFIGDIR"
EOF
)"

# Place startup script
echo "${STARTUP_SCRIPT_STOPSRVS} ${STARTUP_SCRIPT_COPYCONFIG} ${STARTUP_SCRIPT_STARTPR}" >"$BASE_PATH/hyperion_pr$pr_number/$pr_number.sh"

# Set the executen bit
chmod +x -R $BASE_PATH/hyperion_pr$pr_number/$pr_number.sh

echo "*******************************************************************************"
echo "Download finished!"
$REBOOTMESSAGE
echo "You can test the pull request with this command: $BASE_PATH/hyperion_pr$pr_number/$pr_number.sh"
echo "Remove the test installation with: rm -R $BASE_PATH/hyperion_pr$pr_number"
echo "Feedback is welcome at https://github.com/hyperion-project/hyperion.ng/pull/$pr_number"
echo "*******************************************************************************"
