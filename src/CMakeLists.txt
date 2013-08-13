
add_executable(WriteConfig
		WriteConfig.cpp)

add_executable(hyperiond
		hyperion-d.cpp)

target_link_libraries(hyperiond
		hyperion)

# Find the libPNG
find_package(PNG QUIET)

if(PNG_FOUND)
	# Add additional includes dirs
	include_directories(${PNG_INCLUDE_DIR})

	add_executable(ViewPng
			ViewPng.cpp)

	target_link_libraries(ViewPng
			hyperion
			${PNG_LIBRARIES})
endif(PNG_FOUND)

add_subdirectory(hyperion-remote)
