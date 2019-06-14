#!/bin/bash

# detect CI
if [ -n "${TRAVIS-}" ]; then
	# Travis-CI
	CI_NAME="$(echo "$TRAVIS_OS_NAME" | tr '[:upper:]' '[:lower:]')"
	CI_BUILD_DIR="$TRAVIS_BUILD_DIR"
elif [ "$SYSTEM_COLLECTIONID" != "" ]; then
	# Azure Pipelines
	CI_NAME="$(echo "$AGENT_OS" | tr '[:upper:]' '[:lower:]')"
	CI_BUILD_DIR="$BUILD_SOURCESDIRECTORY"
else
	# for executing in non ci environment
	CI_NAME="$(uname -s | tr '[:upper:]' '[:lower:]')"
fi

# sf_upload <FILES> <sf_dir>
# {
# 	echo "Uploading following files: ${1} to dir /hyperion-project/${2}"
#
# }

# append current Date to filename (just packages no binaries)
appendDate()
{
	D=$(date +%Y-%m-%d)
	for F in $CI_BUILD_DIR/deploy/Hy*
	do
		mv "$F" "${F%.*}-$D.${F##*.}"
	done
}

# append friendly name (just packages no binaries)
appendName()
{
	for F in $CI_BUILD_DIR/deploy/Hy*
	do
		mv "$F" "${F%.*}-($DOCKER_NAME).${F##*.}"
	done
}

# get all files to deploy (just packages no binaries)
getFiles()
{
	FILES=""
	for f in $CI_BUILD_DIR/deploy/Hy*;
		do FILES+="${f} ";
	done;
}

if [[ $CI_NAME == 'linux' || "$CI_NAME" == 'osx' || "$CI_NAME" == 'darwin' ]]; then
	if [[ -n $TRAVIS_TAG ]] || [[ $BUILD_SOURCEBRANCH == *"refs/tags"* ]]; then
		echo "tag upload"
		appendName
		appendDate
		getFiles
		# sf_upload $FILES release
	elif [[ $TRAVIS_EVENT_TYPE == 'cron' ]] || [[ $BUILD_REASON == "Schedule" ]]; then
		echo "cron/schedule upload"
		appendName
		appendDate
		getFiles
		# sf_upload $FILES dev/alpha
	else
		echo "Direct pushed no upload, PRs not possible"
	fi
fi
