
add_executable(hyperiond
		hyperiond.cpp)

target_link_libraries(hyperiond
		hyperion
		xbmcvideochecker
		effectengine
		jsonserver
		protoserver
		boblightserver
		v4l2-grabber
)

if (ENABLE_DISPMANX)
	target_link_libraries(hyperiond dispmanx-grabber)
endif (ENABLE_DISPMANX)
