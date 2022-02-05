# Find Broadcom VideoCore firmware installation
#
# This module defines
#  BCM_INCLUDE_DIR - The Broadcom VideoCore include directory
#  BCM_LIBRARY - The Broadcom VideoCore BCM_HOST library.
#  BCM_FOUND - BCM_HOST is available
#

FIND_PATH(BCM_HOST_INCLUDE_DIR bcm_host.h
	PATHS
	/usr/include
	/usr/local/include
	/opt/vc/include
)

FIND_LIBRARY(BCM_HOST_LIBRARY
	NAMES bcm_host
	PATHS
	/usr/lib
	/usr/local/lib
	/opt/vc/lib
)

if (BCM_HOST_INCLUDE_DIR AND BCM_HOST_LIBRARY)
	set(BCM_FOUND TRUE)
endif (BCM_HOST_INCLUDE_DIR AND BCM_HOST_LIBRARY)

if (BCM_FOUND)
	SET (BCM_INCLUDE_DIR
		${BCM_HOST_INCLUDE_DIR} ${BCM_HOST_INCLUDE_DIR}/interface/vcos/pthreads/
		${BCM_HOST_INCLUDE_DIR}/interface/vmcs_host/linux/
	)

	SET (BCM_LIBRARY ${BCM_HOST_LIBRARY})
else (BCM_FOUND)
	SET (BCM_INCLUDE_DIR "")
	SET (BCM_LIBRARY "")
endif (BCM_FOUND)

mark_as_advanced(BCM_INCLUDE_DIR BCM_LIBRARY)
