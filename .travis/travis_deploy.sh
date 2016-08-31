#!/bin/bash

# sf_upload <binarylist> <sf_dir>
sf_upload()
{
	/usr/bin/expect <<EOD
	spawn scp $1 hyperionsf37@frs.sourceforge.net:/home/frs/project/hyperion-project/$2
	expect "*(yes/no)*"
	send "yes\r"
	expect "*password:*"
	send "$SFPW\r"
	expect eof
	EOD
}


if [[ $TRAVIS_OS_NAME == 'linux' ]]; then
	if [[ -n $TRAVIS_TAG ]]; then
		echo "tag upload"
		echo "build dir: ${TRAVIS_BUILD_DIR}"
		sf_upload $TRAVIS_BUILD_DIR/afile nightly
	elif [[ $TRAVIS_EVENT_TYPE == 'cron' ]]; then
		sf_upload abinary nightly
	fi
fi
