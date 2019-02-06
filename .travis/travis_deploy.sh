#!/bin/bash

# sf_upload <FILES> <sf_dir>
sf_upload()
{
echo "Uploading following files: ${1}
to dir /hyperion-project/${2}"

	/usr/bin/expect <<-EOD
	spawn scp $1hyperionsf37@frs.sourceforge.net:/home/frs/project/hyperion-project/$2
	expect "*(yes/no)*"
	send "yes\r"
	expect "*password:*"
	send "$SFPW\r"
	expect eof
	EOD
}

# append current Date to filename (just packages no binaries)
appendDate()
{
	D=$(date +%Y-%m-%d)
	for F in $TRAVIS_BUILD_DIR/deploy/Hy*
	do
		mv "$F" "${F%.*}-$D.${F##*.}"
	done
}

# append friendly name (just packages no binaries)
appendName()
{
	for F in $TRAVIS_BUILD_DIR/deploy/Hy*
	do
		mv "$F" "${F%.*}-($DOCKER_NAME).${F##*.}"
	done
}

# get all files to deploy (just packages no binaries)
getFiles()
{
	FILES=""
	for f in $TRAVIS_BUILD_DIR/deploy/Hy*;
		do FILES+="${f} ";
	done;
}

if [[ $TRAVIS_OS_NAME == 'linux' ]]; then
	if [[ -n $TRAVIS_TAG ]]; then
		echo "tag upload"
		appendName
		appendDate
		getFiles
		sf_upload $FILES release
	elif [[ $TRAVIS_EVENT_TYPE == 'cron' ]]; then
		echo "cron upload"
		appendName
		appendDate
		getFiles
		sf_upload $FILES dev/alpha
	else
		echo "Direct pushed no upload, PRs not possible"
		#sf_upload $FILES pr
	fi
fi
