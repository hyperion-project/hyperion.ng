#!/usr/bin/env bash

# Copied from https://github.com/HEXRD/hexrdgui/blob/master/packaging/hdiutil_repeat.sh
# Workaround XProtect race condition for "hdiutil create" for MacOS 13

 set -e

 if [ "$1" != "create" ]; then
   # If it isn't an `hdiutil create` command, just run and exit normally
   hdiutil "$@"
   exit 0
 fi

 # For an `hdiutil create` command, try repeatedly, up to 10 times
 # This prevents spurious errors caused by a race condition with XProtect
 # See https://github.com/actions/runner-images/issues/7522
 i=0
 until
 hdiutil "$@"
 do
 if [ $i -eq 10 ]; then exit 1; fi
 i=$((i+1))
 sleep 1
 done
