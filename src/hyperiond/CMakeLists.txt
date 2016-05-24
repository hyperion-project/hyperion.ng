
add_executable(hyperiond
		hyperiond.cpp)

target_link_libraries(hyperiond
		hyperion
		xbmcvideochecker
		effectengine
		jsonserver
		boblightserver
)

if (ENABLE_DISPMANX)
	target_link_libraries(hyperiond dispmanx-grabber)
endif ()

if (ENABLE_FB)
	target_link_libraries(hyperiond framebuffer-grabber)
endif ()

if (ENABLE_OSX)
	target_link_libraries(hyperiond osx-grabber)
endif ()

if (ENABLE_V4L2)
	target_link_libraries(hyperiond v4l2-grabber)
endif ()

if (ENABLE_AMLOGIC)
	target_link_libraries(hyperiond amlogic-grabber)
endif ()

if (ENABLE_PROTOBUF)
	target_link_libraries(hyperiond protoserver)
endif ()

install ( TARGETS hyperiond DESTINATION "bin" COMPONENT ambilight )
install ( DIRECTORY ${CMAKE_SOURCE_DIR}/effects     DESTINATION "share/hyperion/" COMPONENT  ambilight )
install ( DIRECTORY ${CMAKE_SOURCE_DIR}/bin/service DESTINATION "share/hyperion/" COMPONENT  ambilight )
install ( DIRECTORY ${CMAKE_SOURCE_DIR}/config      DESTINATION "share/hyperion/" COMPONENT  ambilight )


