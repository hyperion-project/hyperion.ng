add_library(dda-grabber
	${CMAKE_SOURCE_DIR}/include/grabber/dda/DDAGrabber.h
	${CMAKE_SOURCE_DIR}/include/grabber/dda/DDAWrapper.h
	${CMAKE_SOURCE_DIR}/libsrc/grabber/dda/DDAGrabber.cpp
	${CMAKE_SOURCE_DIR}/libsrc/grabber/dda/DDAWrapper.cpp
)

target_link_libraries(dda-grabber
	hyperion
	d3d11.lib
	dxgi.lib
	d2d1.lib
)
