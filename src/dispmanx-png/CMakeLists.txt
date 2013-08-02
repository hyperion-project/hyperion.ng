
# Find the BCM-package (VC control)
find_package(BCM REQUIRED)

# Add the include dirs to the search path
include_directories(${BCM_INCLUDE_DIRS})

add_executable(dispmanx-png
		dispmanx-png.cpp)

target_link_libraries(dispmanx-png
		hyperion-png
		${BCM_LIBRARIES})
