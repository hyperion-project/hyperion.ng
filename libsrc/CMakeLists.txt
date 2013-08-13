
# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc)

add_library(bob2hyperion SHARED
		bob2hyperion.cpp)

target_link_libraries(bob2hyperion
		hyperion
		hyperion-utils)

add_subdirectory(hyperion)
add_subdirectory(utils)

# Find the libPNG
find_package(PNG REQUIRED QUIET)

if (PNG_FOUND)
	# Add additional includes dirs
	include_directories(${PNG_INCLUDE_DIR})

	add_library(bob2hyperion-png SHARED
			hyperion-png.cpp)

	target_link_libraries(bob2hyperion-png
			hyperion-png)

	add_subdirectory(hyperionpng)
endif(PNG_FOUND)

