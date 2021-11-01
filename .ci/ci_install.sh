#!/bin/bash

# detect CI
if [ "$HOME" != "" ]; then
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
		list_output=`brew list --formula | grep $i`
		outdated_output=`brew outdated | grep $i`

		if [[ ! -z "$list_output" ]]; then
		    if [[ ! -z "$outdated_output" ]]; then
		    	echo "Outdated package: ${outdated_output}"
		    	brew unlink  ${outdated_output}
			brew upgrade $i
			brew link --overwrite $i
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
