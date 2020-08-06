# -----------------------------------------------------------------------------
# Find IOKit framework (Mac OS X).
#
# Define:
# IOKit_FOUND
# IOKit_INCLUDE_DIR
# IOKit_LIBRARY

set(IOKit_FOUND false)
set(IOKit_INCLUDE_DIR)
set(IOKit_LIBRARY)

if(APPLE)
# The only platform it makes sense to check for IOKit
	find_library(IOKit IOKit)
	if(IOKit)
		set(IOKit_FOUND true)
		set(IOKit_INCLUDE_DIR ${IOKit})
		set(IOKit_LIBRARY ${IOKit})
	endif(IOKit)
endif(APPLE)
