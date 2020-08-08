# -----------------------------------------------------------------------------
# Find IOKit framework (Mac OS X).
#
# Define:
# CoreFoundation_FOUND
# CoreFoundation_INCLUDE_DIR
# CoreFoundation_LIBRARY

set(CoreFoundation_FOUND false)
set(CoreFoundation_INCLUDE_DIR)
set(CoreFoundation_LIBRARY)

if(APPLE)
# The only platform it makes sense to check for CoreFoundation
	find_library(CoreFoundation CoreFoundation)
	if(CoreFoundation)
		set(CoreFoundation_FOUND true)
		set(CoreFoundation_INCLUDE_DIR ${CoreFoundation})
		set(CoreFoundation_LIBRARY ${CoreFoundation})
	endif()
endif()
