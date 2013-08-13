
add_executable(WriteConfig
		WriteConfig.cpp)

add_executable(hyperion-d
		hyperion-d.cpp)

target_link_libraries(hyperion-d
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
