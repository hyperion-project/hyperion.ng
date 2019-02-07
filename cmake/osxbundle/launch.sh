#!/bin/sh
cd "$(dirname "$0")"
# Path to hyperiond!?
cd ../Resources/bin
exec ./hyperiond "$@"
