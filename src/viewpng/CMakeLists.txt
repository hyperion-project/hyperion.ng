# Find the libPNG
find_package(PNG QUIET)

if(PNG_FOUND)
	# Add additional includes dirs
	include_directories(${PNG_INCLUDE_DIR})

	add_executable(viewpng
			ViewPng.cpp)

	target_link_libraries(viewpng
			hyperion
			${PNG_LIBRARIES})
endif(PNG_FOUND)
