#!/bin/bash

# sf_upload <binarylist> <sf_dir>
sf_upload()
{
	/usr/bin/expect <<-EOD
	spawn scp $1 hyperionsf37@frs.sourceforge.net:/home/frs/project/hyperion-project/$2
	expect "*(yes/no)*"
	send "yes\r"
	expect "*password:*"
	send "$SFPW\r"
	expect eof
	EOD
}

if [[ $TRAVIS_OS_NAME == 'linux' ]]; then
	cd $TRAVIS_BUILD_DIR/build
	if [[ -n $TRAVIS_TAG ]]; then
		echo "tag upload"
		sf_upload hyperion-2.0.0-Linux-x86.deb release
	elif [[ $TRAVIS_EVENT_TYPE == 'cron' ]]; then
		echo "cron upload"
		sf_upload hyperion-2.0.0-Linux-x86.deb nightly
	else
		echo "PR upload"
		sf_upload hyperion-2.0.0-Linux-x86-dev.deb pullrequest
	fi
fi
