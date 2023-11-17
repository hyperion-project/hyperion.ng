find_package(X11 REQUIRED)

add_library(x11-grabber
	${CMAKE_SOURCE_DIR}/include/grabber/x11/X11Grabber.h
	${CMAKE_SOURCE_DIR}/include/grabber/x11/X11Wrapper.h
	${CMAKE_SOURCE_DIR}/libsrc/grabber/x11/X11Grabber.cpp
	${CMAKE_SOURCE_DIR}/libsrc/grabber/x11/X11Wrapper.cpp
)

target_link_libraries(x11-grabber
	hyperion
	${X11_LIBRARIES}
	${X11_Xrandr_LIB}
	${X11_Xrender_LIB}
)

if(APPLE)
	list(APPEND X11_INCLUDES "/opt/X11/include")
endif()

target_include_directories(x11-grabber PUBLIC
	${X11_INCLUDES}
)
