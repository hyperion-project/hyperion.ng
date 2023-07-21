if (APPLE)
	project(Hyperion)
else()
	project(hyperiond)
endif()

if(ENABLE_EFFECTENGINE)
	if (NOT CMAKE_VERSION VERSION_LESS "3.12")
		find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
		include_directories(${Python3_INCLUDE_DIRS} ${Python3_INCLUDE_DIRS}/..)
	else()
		find_package (PythonLibs ${PYTHON_VERSION_STRING} EXACT) # Maps PythonLibs to the PythonInterp version of the main cmake
		include_directories(${PYTHON_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS}/..)
	endif()
endif ()

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Network Gui Widgets REQUIRED)

# generate windows .rc file for this binary
if (WIN32)
	include(${CMAKE_SOURCE_DIR}/cmake/win/win_rc.cmake)
	generate_win_rc_file(${PROJECT_NAME})
endif(WIN32)

# include resource files for macos bundle (copy LICENSE file and correct line breaks)
if (APPLE)
	configure_file(${CMAKE_SOURCE_DIR}/LICENSE ${CMAKE_BINARY_DIR}/LICENSE COPYONLY)
	execute_process(COMMAND bash -c "perl -pi -e 's/\n/\r/g' ${CMAKE_BINARY_DIR}/LICENSE")
	set(BUNDLE_RESOURCE_FILES ${CMAKE_SOURCE_DIR}/cmake/osxbundle/Hyperion.icns ${CMAKE_BINARY_DIR}/LICENSE)
	set_source_files_properties(${BUNDLE_RESOURCE_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
endif(APPLE)

if (UNIX)
	find_package(Qt${QT_VERSION_MAJOR} COMPONENTS DBus QUIET )
	if (Qt${QT_VERSION_MAJOR}DBus_FOUND)
		set(hyperiond_POWER_MNG_DBUS "Qt${QT_VERSION_MAJOR}::DBus")
	endif()
endif(UNIX)

add_executable(${PROJECT_NAME}
	console.h
	hyperiond.h
	systray.h
	hyperiond.cpp
	systray.cpp
	SuspendHandler.cpp
	main.cpp
	${hyperiond_WIN_RC_PATH}
	${BUNDLE_RESOURCE_FILES}
)

if (UNIX AND NOT APPLE AND Qt${QT_VERSION_MAJOR}DBus_FOUND)
	target_compile_definitions(${PROJECT_NAME} PUBLIC HYPERION_HAS_DBUS)
endif()

# promote hyperiond as GUI app
if (WIN32)
	target_link_options(${PROJECT_NAME} PUBLIC /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup)
endif(WIN32)

target_link_libraries(${PROJECT_NAME}
	commandline
	hyperion
	jsonserver
	webserver
	ssdp
	database
	resources
	Qt${QT_VERSION_MAJOR}::Core
	Qt${QT_VERSION_MAJOR}::Gui
	Qt${QT_VERSION_MAJOR}::Network
	Qt${QT_VERSION_MAJOR}::Widgets
	${hyperiond_POWER_MNG_DBUS}
)

if(ENABLE_EFFECTENGINE)
	target_link_libraries(${PROJECT_NAME} effectengine python)

	if (NOT CMAKE_VERSION VERSION_LESS "3.12")
		target_link_libraries( ${PROJECT_NAME} ${Python3_LIBRARIES} )
	else()
		target_link_libraries( ${PROJECT_NAME} ${PYTHON_LIBRARIES} )
	endif()
endif ()

if(ENABLE_FLATBUF_SERVER)
	target_link_libraries(${PROJECT_NAME} flatbufserver)
endif()

if(ENABLE_PROTOBUF_SERVER)
	target_link_libraries(${PROJECT_NAME} protoserver)
endif()

if (ENABLE_AMLOGIC)
	target_link_libraries(${PROJECT_NAME}
		#Qt${QT_VERSION_MAJOR}::Core
		pcre16 dl z
	)
endif(ENABLE_AMLOGIC)

if (ENABLE_DISPMANX)
	target_link_libraries(${PROJECT_NAME} dispmanx-grabber)
endif (ENABLE_DISPMANX)

if (ENABLE_FB)
	target_link_libraries(${PROJECT_NAME} framebuffer-grabber)
endif (ENABLE_FB)

if (ENABLE_OSX)
	target_link_libraries(${PROJECT_NAME} osx-grabber)
endif (ENABLE_OSX)

if (ENABLE_V4L2)
	target_link_libraries(${PROJECT_NAME} v4l2-grabber)
endif ()

if (ENABLE_MF)
	target_link_libraries(${PROJECT_NAME} mf-grabber)
endif (ENABLE_MF)

if (ENABLE_AUDIO)
	target_link_libraries(hyperiond audio-grabber)
endif()

if (ENABLE_AMLOGIC)
	target_link_libraries(${PROJECT_NAME} amlogic-grabber)
endif (ENABLE_AMLOGIC)

if (ENABLE_X11)
	if(APPLE)
		include_directories("/opt/X11/include")
	endif(APPLE)
	target_link_libraries(${PROJECT_NAME} x11-grabber)
endif (ENABLE_X11)

if (ENABLE_XCB)
	target_link_libraries(${PROJECT_NAME} xcb-grabber)
endif (ENABLE_XCB)

if (ENABLE_QT)
	target_link_libraries(${PROJECT_NAME} qt-grabber)
endif (ENABLE_QT)

if (ENABLE_DX)
	include_directories(${DIRECTX9_INCLUDE_DIRS})
	target_link_libraries(${PROJECT_NAME} directx-grabber)
endif (ENABLE_DX)

if (ENABLE_CEC)
        target_link_libraries(${PROJECT_NAME} cechandler)
endif (ENABLE_CEC)

if (ENABLE_MDNS)
	target_link_libraries(${PROJECT_NAME} mdns)
endif()

if (APPLE)
	set_target_properties( ${PROJECT_NAME} PROPERTIES
		MACOSX_BUNDLE TRUE
		MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/cmake/osxbundle/Info.plist.in
		MACOSX_BUNDLE_BUNDLE_NAME "Hyperion"
		MACOSX_BUNDLE_BUNDLE_VERSION ${HYPERION_VERSION}
		MACOSX_BUNDLE_COPYRIGHT "Copyright(c) 2014-2023 Hyperion Project"
		MACOSX_BUNDLE_GUI_IDENTIFIER "com.hyperion-project.${PROJECT_NAME}"
		MACOSX_BUNDLE_ICON_FILE "Hyperion.icns"
		MACOSX_BUNDLE_INFO_STRING "${PROJECT_NAME} ${HYPERION_VERSION}"
		MACOSX_BUNDLE_SHORT_VERSION_STRING ${HYPERION_VERSION}
		MACOSX_BUNDLE_LONG_VERSION_STRING ${HYPERION_VERSION}
	)

	install ( TARGETS ${PROJECT_NAME} DESTINATION . COMPONENT "Hyperion")
elseif(NOT WIN32)
	install ( TARGETS ${PROJECT_NAME} DESTINATION "share/hyperion/bin" COMPONENT "Hyperion" )
	install ( DIRECTORY ${CMAKE_SOURCE_DIR}/bin/service DESTINATION "share/hyperion/" COMPONENT "Hyperion" )
	install ( FILES ${CMAKE_SOURCE_DIR}/effects/readme.txt DESTINATION "share/hyperion/effects" COMPONENT "Hyperion" )
	install ( FILES ${CMAKE_SOURCE_DIR}/resources/icons/hyperion-icon-32px.png DESTINATION "share/hyperion/icons" COMPONENT "Hyperion" )

	# Desktop file for Hyperion
	install ( FILES ${CMAKE_SOURCE_DIR}/cmake/desktop/hyperiond_128.png DESTINATION "share/hyperion/desktop" COMPONENT "Hyperion" )
	install ( FILES ${CMAKE_SOURCE_DIR}/cmake/desktop/hyperiond.desktop DESTINATION "share/hyperion/desktop" COMPONENT "Hyperion" )
else()
	install ( TARGETS ${PROJECT_NAME} DESTINATION "bin" COMPONENT "Hyperion" )
	install ( FILES ${CMAKE_SOURCE_DIR}/effects/readme.txt DESTINATION "effects" COMPONENT "Hyperion" )

	#set( CMAKE_INSTALL_UCRT_LIBRARIES TRUE )
	#set( CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE )
	#include( InstallRequiredSystemLibraries )
endif()

if (CMAKE_HOST_UNIX AND NOT APPLE)
	install( CODE "EXECUTE_PROCESS(COMMAND ln -sf \"../share/hyperion/bin/${PROJECT_NAME}\" \"${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}\" )" COMPONENT "Hyperion" )
	install( FILES "${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}" DESTINATION "bin" RENAME ${PROJECT_NAME} COMPONENT "Hyperion" )
	install( CODE "FILE (REMOVE ${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME})" COMPONENT "Hyperion" )
	install( FILES ${CMAKE_SOURCE_DIR}/bin/scripts/updateHyperionUser.sh DESTINATION "share/hyperion/scripts" COMPONENT "Hyperion" )
endif()

# Deploy Qt DLLs into the binary folder.
# This is necessary for starting the application from within the IDE

if (WIN32)
	get_target_property(QT_QMAKE_EXECUTABLE Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
	get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
	find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_BIN_DIR}")

	if (NOT WINDEPLOYQT_EXECUTABLE)
		find_program(WINDEPLOYQT_EXECUTABLE windeployqt)
	endif()

	if (WINDEPLOYQT_EXECUTABLE AND NOT CMAKE_GITHUB_ACTION)
		set(WINDEPLOYQT_PARAMS_RUNTIME --verbose 0 --no-compiler-runtime --no-opengl-sw --no-system-d3d-compiler)
		message(STATUS "Found windeployqt: ${WINDEPLOYQT_EXECUTABLE} PATH_HINT:${QT_BIN_DIR}")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${WINDEPLOYQT_EXECUTABLE} ${WINDEPLOYQT_PARAMS_RUNTIME} "$<TARGET_FILE:${PROJECT_NAME}>")
	endif()

	find_package(OpenSSL REQUIRED)
	if (OPENSSL_FOUND)
		string(REGEX MATCHALL "[0-9]+" openssl_versions "${OPENSSL_VERSION}")
		list(GET openssl_versions 0 openssl_version_major)
		list(GET openssl_versions 1 openssl_version_minor)

		set(library_suffix "-${openssl_version_major}_${openssl_version_minor}")
		if (CMAKE_SIZEOF_VOID_P EQUAL 8)
			string(APPEND library_suffix "-x64")
		endif()

		find_file(OPENSSL_SSL
			NAMES "libssl${library_suffix}.dll"
			PATHS ${OPENSSL_INCLUDE_DIR}/.. ${OPENSSL_INCLUDE_DIR}/../bin
			NO_DEFAULT_PATH
		)

		find_file(OPENSSL_CRYPTO
			NAMES "libcrypto${library_suffix}.dll"
			PATHS ${OPENSSL_INCLUDE_DIR}/.. ${OPENSSL_INCLUDE_DIR}/../bin
			NO_DEFAULT_PATH
		)

		add_custom_command(
			TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy
			${OPENSSL_SSL} ${OPENSSL_CRYPTO}
			$<TARGET_FILE_DIR:${PROJECT_NAME}>
		)
	endif(OPENSSL_FOUND)
endif(WIN32)

if(ENABLE_DEPLOY_DEPENDENCIES)
	# Deploy all dependencies for package creation
	include(${CMAKE_SOURCE_DIR}/cmake/Dependencies.cmake)

	if (APPLE) #macOS
		DeployMacOS(${PROJECT_NAME})
	elseif (NOT WIN32) # Linux
		DeployLinux(${PROJECT_NAME})
	elseif(WIN32) # Windows
		DeployWindows(${PROJECT_NAME})
	endif ()
endif(ENABLE_DEPLOY_DEPENDENCIES)
