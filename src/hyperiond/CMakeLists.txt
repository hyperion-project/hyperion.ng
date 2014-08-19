
add_executable(hyperiond
		hyperiond.cpp)

target_link_libraries(hyperiond
		hyperion
		xbmcvideochecker
		effectengine
		jsonserver
		protoserver
		boblightserver
)

if (ENABLE_DISPMANX)
	target_link_libraries(hyperiond dispmanx-grabber)
endif (ENABLE_DISPMANX)

if (ENABLE_V4L2)
	target_link_libraries(hyperiond v4l2-grabber)
endif (ENABLE_V4L2)
