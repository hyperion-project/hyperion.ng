
add_executable(hyperiond
		hyperiond.cpp)

target_link_libraries(hyperiond
		bootsequence
		hyperion
		xbmcvideochecker
		jsonserver
		protoserver
		boblightserver)

if (ENABLE_DISPMANX)
	target_link_libraries(hyperiond dispmanx-grabber)
endif (ENABLE_DISPMANX)
