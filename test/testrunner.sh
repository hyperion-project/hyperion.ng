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

######################################
## EXEC TESTS
cd build || exit 1

echo
echo "Hyperion test execution"
echo
exec_test "hyperiond is executable and show version" bin/hyperiond --version
exec_test "test config" bin/test_configfile ../config/hyperion.config.json.commented
exec_test "test config" bin/test_configfile ../config/hyperion.config.json.default

echo
echo
echo "TEST SUMMARY"
echo "============"
echo "    total: $STATS_TOTAL"
echo "  success: $STATS_SUCCESS"
echo "   failed: $STATS_FAILED"

[ $STATS_FAILED -gt 0 ] && exit 200
exit 0

 
