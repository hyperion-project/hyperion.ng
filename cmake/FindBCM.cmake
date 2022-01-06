# Find Broadcom VideoCore firmware installation
#
# This module defines
#  BCM_INCLUDE_DIR - The Broadcom VideoCore include directory
#  BCM_LIBRARIES - The Broadcom VideoCore BCM_HOST library.
#  BCM_FOUND - BCM_HOST is available
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
