#!/bin/bash

# sf_upload <deploylist> <sf_dir>
sf_upload()
{
	/usr/bin/expect <<-EOD
	spawn scp $1 hyperionsf37@frs.sourceforge.net:/home/frs/project/hyperion-project/dev/$2
	expect "*(yes/no)*"
	send "yes\r"
	expect "*password:*"
	send "$SFPW\r"
	expect eof
	EOD
}

deploylist="hyperion-2.0.0-Linux-x86.tar.gz"

if [[ $TRAVIS_OS_NAME == 'linux' ]]; then
	cd $TRAVIS_BUILD_DIR/build
	if [[ -n $TRAVIS_TAG ]]; then
		echo "tag upload"
		sf_upload $deploylist release
	elif [[ $TRAVIS_EVENT_TYPE == 'cron' ]]; then
		echo "cron upload"
		sf_upload $deploylist alpha
	else
		echo "PR can't be uploaded for security reasons"
		sf_upload $deploylist pr
	fi
fi
