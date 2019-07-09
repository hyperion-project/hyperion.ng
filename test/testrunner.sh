#!/bin/bash

[ "${BUILD_TYPE}" == "Release" ] && exit 0

STATS_FAILED=0
STATS_SUCCESS=0
STATS_SKIPPED=0
STATS_TOTAL=0

# exec_test "test name" test_exec --with --args
function exec_test()
{
	local test_name="$1"
	if [ ! -e "$2" ]
	then
		echo "skip test: '$test_name'"
		(( STATS_SKIPPED++ ))
		return
	fi
	shift
	(( STATS_TOTAL++ ))
	echo "execute test: '$test_name'"
	if $@
	then
		echo -e "   ... success"
		(( STATS_SUCCESS++ ))
		return 0
	else
		echo -e "   ... failed"
		(( STATS_FAILED++ ))
		return 1
	fi
	echo
}

######################################
############# EXEC TESTS #############
######################################

cd build || exit 1

echo
echo "Hyperion test execution"
echo
exec_test "hyperiond is executable and show version" bin/hyperiond --version

for cfg in ../config/*json.default
do
	exec_test "test $(basename $cfg)" bin/test_configfile $cfg
done

echo
echo
echo "TEST SUMMARY"
echo "============"
echo "  total: $STATS_TOTAL"
echo "success: $STATS_SUCCESS"
echo "skipped: $STATS_SKIPPED"
echo " failed: $STATS_FAILED"

sleep 2

[ $STATS_FAILED -gt 0 ] && exit 200
exit 0
