# Common cmake definition for external video grabber

set(MF-grabber mediafoundation)
set(V4L2-grabber v4l2)

if(ENABLE_MF)
	project(mf-grabber)
	set(grabber_project MF)
	set(MediaFoundationSourceReaderCallBack ${CMAKE_SOURCE_DIR}/libsrc/grabber/video/mediafoundation/MFSourceReaderCB.h)
elseif(ENABLE_V4L2)
	project(v4l2-grabber)
	set(grabber_project V4L2)
endif()

if(ENABLE_V4L2 OR ENABLE_MF)
	add_library(${PROJECT_NAME}
		${CMAKE_SOURCE_DIR}/include/grabber/video/EncoderThread.h
		${CMAKE_SOURCE_DIR}/include/grabber/video/VideoWrapper.h
		${CMAKE_SOURCE_DIR}/libsrc/grabber/video/EncoderThread.cpp
		${CMAKE_SOURCE_DIR}/libsrc/grabber/video/VideoWrapper.cpp
		${CMAKE_SOURCE_DIR}/include/grabber/video/${${grabber_project}-grabber}/${grabber_project}Grabber.h
		${CMAKE_SOURCE_DIR}/libsrc/grabber/video/${${grabber_project}-grabber}/${grabber_project}Grabber.cpp
		${MediaFoundationSourceReaderCallBack}
	)

	target_link_libraries(${PROJECT_NAME} hyperion)

	if(ENABLE_MF AND USE_PRE_BUILT_DEPS)
		set(TURBOJPEG_ROOT_DIR ${PRE_BUILT_DEPS_DIR})
	endif()

	find_package(TurboJPEG)
	if(TURBOJPEG_FOUND AND TARGET turbojpeg)
		target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_TURBO_JPEG)
		target_link_libraries(${PROJECT_NAME} turbojpeg)
	else ()
		message(STATUS "TurboJPEG library not found, MJPEG camera format won't work.")
	endif()
endif()

