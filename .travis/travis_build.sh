#!/bin/bash

STATS_FAILED=0
STATS_SUCCESS=0
STATS_TOTAL=0


# exec_test "test name" test_exec --with --args
function exec_test()
{
	local test_name="$1"
	shift
	(( STATS_TOTAL++ ))
	echo "execute test: '$test_name'"
	if $@
	then
		echo -e "   ... success"
		(( STATS_SUCCESS++ ))
	else
		echo -e "   ... failed"
		(( STATS_FAILED++ ))
	fi
	echo
}

# for executing in non travis environment
[ -z "$TRAVIS_OS_NAME" ] && TRAVIS_OS_NAME="$( uname -s | tr '[:upper:]' '[:lower:]' )"


######################################
## COMPILE HYPERION

# compile hyperion on osx
if [[ $TRAVIS_OS_NAME == 'osx' ]]
then
	cmake . -DCMAKE_PREFIX_PATH=/usr/local/Cellar/qt5/5.6.1-1
	mkdir build || exit 1
	cd build
	cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_TESTS=ON -Wno-dev .. || exit 2
	make -j$(nproc) || exit 3
	# make -j$(nproc) package || exit 4 # currently osx(dmg) package creation not implemented
fi

# compile hyperion on linux
if [[ $TRAVIS_OS_NAME == 'linux' ]]
then
	mkdir build || exit 1
	cd build
	cmake -DPLATFORM=x86 -DCMAKE_BUILD_TYPE=Release -DENABLE_AMLOGIC=ON -DENABLE_TESTS=ON -DENABLE_SPIDEV=ON -DENABLE_WS281XPWM=ON .. || exit 2
	make -j$(nproc) || exit 3
	make -j$(nproc) package || exit 4
fi


######################################
## EXEC TESTS

echo
echo "Hyperion test execution"
echo
exec_test "hyperiond is executable and show version" bin/hyperiond --version

echo
echo
echo "TEST SUMMARY"
echo "============"
echo "    total: $STATS_TOTAL"
echo "  success: $STATS_SUCCESS"
echo "   failed: $STATS_FAILED"

[ $STATS_FAILED -gt 0 ] && exit 200
exit 0

