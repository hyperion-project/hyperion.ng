#!/bin/sh

# Fail on error.
set -e

# Log file containing documentation errors and warnings (if any).
log_file=${CMAKE_CURRENT_BINARY_DIR}/hyperion-doxygen.log

# Remove the log file before building the documentation.
rm -f $log_file

# Generate the documentation.
${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/hyperion.doxygen

# At this point, the log file should have been generated.
# If not, an error is displayed on stderr and 1 is returned to indicate an error.
if [ -f $log_file ] ; then
	# So the log file exists. If its size is > 0, show its contents on stderr and exit 1.
	if [ -s $log_file ] ; then
	cat $log_file 1>&2
	exit 1;
	else
	# The log file exists, but its size is zero, meaning there were no documentation warnings or errors.
	# Exit with 0 to indicate success.
	exit 0;
	fi
else
	echo "The doxygen log file ($log_file) does not exist. Ensure that WARN_LOGFILE is set correctly in hyperion-cmake.doxyfile." 1>&2
	exit 1;
fi
