# - Find the native BCM includes and library
#

# This module defines
#  BCM_INCLUDE_DIR, where to find png.h, etc.
#  BCM_LIBRARIES, the libraries to link against to use PNG.
#  BCM_FOUND, If false, do not try to use PNG.
#

FIND_PATH(BCM_INCLUDE_DIR
	bcm_host.h
	/usr/include
	/usr/local/include
	/opt/vc/include)

SET(BCM_INCLUDE_DIRS
	${BCM_INCLUDE_DIR}
	${BCM_INCLUDE_DIR}/interface/vcos/pthreads
	${BCM_INCLUDE_DIR}/interface/vmcs_host/linux)

FIND_LIBRARY(BCM_LIBRARIES
	NAMES bcm_host
	PATHS /usr/lib /usr/local/lib /opt/vc/lib)
