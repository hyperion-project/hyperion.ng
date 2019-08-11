find_package(PythonLibs 3.4 REQUIRED)
find_package(Qt5Widgets REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS}/..)

add_executable(hyperiond
	hyperiond.h
	systray.h
	hyperiond.cpp
	systray.cpp
	main.cpp
)

target_link_libraries(hyperiond
		commandline
		hyperion
		effectengine
		jsonserver
		flatbufserver
		protoserver
		webserver
		bonjour
		ssdp
		database
		python
		resources
		${PYTHON_LIBRARIES}
		Qt5::Widgets
)

if (ENABLE_AMLOGIC)
	target_link_libraries(hyperiond
		Qt5::Core
		pcre16 dl z
	)
endif()

if (ENABLE_DISPMANX)
	IF ( "${PLATFORM}" MATCHES rpi)
		find_package(BCM REQUIRED)
		include_directories(${BCM_INCLUDE_DIRS})
	ELSE()
		SET(BCM_INCLUDE_DIRS "")
		SET(BCM_LIBRARIES "")
	ENDIF()
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

if (ENABLE_X11)
	target_link_libraries(hyperiond	x11-grabber)
endif ()

if (ENABLE_QT)
	target_link_libraries(hyperiond	qt-grabber)
endif ()

install ( TARGETS hyperiond DESTINATION "share/hyperion/bin/" COMPONENT "${PLATFORM}" )
install ( DIRECTORY ${CMAKE_SOURCE_DIR}/bin/service DESTINATION "share/hyperion/" COMPONENT  "${PLATFORM}" )
install ( FILES ${CMAKE_SOURCE_DIR}/effects/readme.txt DESTINATION "share/hyperion/effects" COMPONENT  "${PLATFORM}" )
install ( FILES ${CMAKE_SOURCE_DIR}/resources/icons/hyperion-icon-32px.png DESTINATION "share/hyperion/icons" COMPONENT "${PLATFORM}" )

# Desktop file for hyperiond
install ( FILES ${CMAKE_SOURCE_DIR}/cmake/desktop/hyperiond_128.png DESTINATION "share/hyperion/desktop" COMPONENT "${PLATFORM}" )
install ( FILES ${CMAKE_SOURCE_DIR}/cmake/desktop/hyperiond.desktop DESTINATION "share/hyperion/desktop" COMPONENT "${PLATFORM}" )

if(CMAKE_HOST_UNIX)
	install(CODE "EXECUTE_PROCESS(COMMAND ln -sf \"../share/hyperion/bin/hyperiond\" \"${CMAKE_BINARY_DIR}/symlink_hyperiond\" )" COMPONENT  "${PLATFORM}" )
	install(FILES ${CMAKE_BINARY_DIR}/symlink_hyperiond DESTINATION "bin" RENAME hyperiond COMPONENT "${PLATFORM}" )
	install(CODE "FILE (REMOVE ${CMAKE_BINARY_DIR}/symlink_hyperiond )" COMPONENT "${PLATFORM}" )
endif(CMAKE_HOST_UNIX)
