
# Find the libPNG
find_package(PNG REQUIRED QUIET)

# Add additional includes dirs
include_directories(${PNG_INCLUDE_DIR})

# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/hyperionpng)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/hyperionpng)

# Create the 'rasplight-png' library
add_library(hyperion-png SHARED
		${CURRENT_HEADER_DIR}/HyperionPng.h
		${CURRENT_SOURCE_DIR}/HyperionPng.cpp
		${CURRENT_SOURCE_DIR}/pngwriter.h
		${CURRENT_SOURCE_DIR}/pngwriter.cc)

target_link_libraries(hyperion-png
		hyperion-utils
		${PNG_LIBRARIES})
