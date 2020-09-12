#!/bin/bash

# detect CI
if [ "$SYSTEM_COLLECTIONID" != "" ]; then
	# Azure Pipelines
	CI_NAME="$(echo "$AGENT_OS" | tr '[:upper:]' '[:lower:]')"
	CI_BUILD_DIR="$BUILD_SOURCESDIRECTORY"
elif [ "$HOME" != "" ]; then
	# GitHub Actions
	CI_NAME="$(uname -s | tr '[:upper:]' '[:lower:]')"
	CI_BUILD_DIR="$GITHUB_WORKSPACE"
else
	# for executing in non ci environment
	CI_NAME="$(uname -s | tr '[:upper:]' '[:lower:]')"
fi

function installAndUpgrade()
{
	arr=("$@")
	for i in "${arr[@]}";
	do
		list_output=`brew list | grep $i`
		outdated_output=`brew outdated | grep $i`

		if [[ ! -z "$list_output" ]]; then
		    if [[ ! -z "$outdated_output" ]]; then
			brew upgrade $i
		    fi
		else
		    brew install $i
		fi
	done
}

# install osx deps for hyperion compile
if [[ $CI_NAME == 'osx' || $CI_NAME == 'darwin' ]]; then
	echo "Install dependencies"
	brew update
	dependencies=("qt5" "python" "libusb" "cmake" "doxygen")
	installAndUpgrade "${dependencies[@]}"
fi
