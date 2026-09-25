# Shared header-only scaffolding for the standalone grabber apps below
# (GrabberApp.h, GrabberRunner.h, ScreenshotUtil.h). Each grabber's
# CMakeLists.txt adds this as an include directory.
set(GRABBERAPP_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(ENABLE_AMLOGIC AND ENABLE_FLATBUF_CONNECT)
	add_subdirectory(hyperion-aml)
endif()

if(ENABLE_DISPMANX AND ENABLE_FLATBUF_CONNECT)
	add_subdirectory(hyperion-dispmanx)
endif()

if(ENABLE_DRM AND ENABLE_FLATBUF_CONNECT)
	add_subdirectory(hyperion-drm)
endif()

if(ENABLE_FB AND ENABLE_FLATBUF_CONNECT)
	add_subdirectory(hyperion-framebuffer)
endif()

if(ENABLE_OSX AND ENABLE_FLATBUF_CONNECT)
	add_subdirectory(hyperion-osx)
endif()

if(ENABLE_QT AND ENABLE_FLATBUF_CONNECT)
	add_subdirectory(hyperion-qt)
endif()

if(ENABLE_V4L2 AND ENABLE_FLATBUF_CONNECT)
	add_subdirectory(hyperion-v4l2)
endif()

if(ENABLE_X11 AND ENABLE_FLATBUF_CONNECT)
	add_subdirectory(hyperion-x11)
endif()

if(ENABLE_XCB AND ENABLE_FLATBUF_CONNECT)
	add_subdirectory(hyperion-xcb)
endif()
