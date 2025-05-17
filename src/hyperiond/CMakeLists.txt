project(hyperiond)

#####################################
############ Preparation ############
#####################################

get_target_property(QMAKE_EXECUTABLE Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)

if(WIN32)
	# generate windows .rc file for this binary
	string(REPLACE "/" "\\\\" WIN_RC_ICON_PATH ${CMAKE_SOURCE_DIR}/cmake/windows/nsis/installer.ico)
	configure_file(${CMAKE_SOURCE_DIR}/cmake/windows/win.rc.in ${CMAKE_BINARY_DIR}/win.rc)
	set(WIN_RC_FILE ${CMAKE_BINARY_DIR}/win.rc)

	# promote hyperiond as GUI app
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")

	# Search for windeployqt target (qt6 has the target natively) otherwise create it.
	if(NOT TARGET Qt${QT_VERSION_MAJOR}::windeployqt)
		get_filename_component(QT_BIN_DIR ${QMAKE_EXECUTABLE} DIRECTORY)
		find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt HINTS ${QT_BIN_DIR})
		if(EXISTS ${WINDEPLOYQT_EXECUTABLE})
			add_executable(Qt${QT_VERSION_MAJOR}::windeployqt IMPORTED)
			set_target_properties(Qt${QT_VERSION_MAJOR}::windeployqt PROPERTIES
				IMPORTED_LOCATION ${WINDEPLOYQT_EXECUTABLE}
			)
		else()
			message(WARNING "Cannot find the windeployqt tool.")
		endif()
	endif()
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	# include resource files for macos bundle (copy LICENSE file and correct line breaks)
	configure_file(${CMAKE_SOURCE_DIR}/LICENSE ${CMAKE_BINARY_DIR}/LICENSE COPYONLY)
	execute_process(COMMAND bash -c "perl -pi -e 's/\n/\r/g' ${CMAKE_BINARY_DIR}/LICENSE")
	set(MACOS_BUNDLE_RESOURCE_FILES ${CMAKE_SOURCE_DIR}/cmake/macos/Hyperion.icns ${CMAKE_BINARY_DIR}/LICENSE)
	set_source_files_properties(${MACOS_BUNDLE_RESOURCE_FILES} PROPERTIES
		MACOSX_PACKAGE_LOCATION Resources
	)
endif()

#####################################
########### General steps ###########
#####################################

add_executable(${PROJECT_NAME}
	console.h
	hyperiond.h
	systray.h
	hyperiond.cpp
	systray.cpp
	main.cpp
	${WIN_RC_FILE}
	${MACOS_BUNDLE_RESOURCE_FILES}
)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS DBus QUIET)
target_link_libraries(${PROJECT_NAME}
	commandline
	hyperion
	jsonserver
	webserver
	ssdp
	database
	resources
	events
	# Qt
	Qt${QT_VERSION_MAJOR}::Core
	Qt${QT_VERSION_MAJOR}::Gui
	Qt${QT_VERSION_MAJOR}::Network
	Qt${QT_VERSION_MAJOR}::Widgets
	$<$<BOOL:${Qt${QT_VERSION_MAJOR}DBus_FOUND}>:Qt${QT_VERSION_MAJOR}::DBus>
	# Grabber
	$<$<BOOL:${ENABLE_DISPMANX}>:dispmanx-grabber>
	$<$<BOOL:${ENABLE_FB}>:framebuffer-grabber>
	$<$<BOOL:${ENABLE_OSX}>:osx-grabber>
	$<$<BOOL:${ENABLE_V4L2}>:v4l2-grabber>
	$<$<BOOL:${ENABLE_MF}>:mf-grabber>
	$<$<BOOL:${ENABLE_AUDIO}>:audio-grabber>
	$<$<BOOL:${ENABLE_AMLOGIC}>:amlogic-grabber>
	$<$<BOOL:${ENABLE_X11}>:x11-grabber>
	$<$<BOOL:${ENABLE_XCB}>:xcb-grabber>
	$<$<BOOL:${ENABLE_QT}>:qt-grabber>
	$<$<BOOL:${ENABLE_DX}>:directx-grabber>
	$<$<BOOL:${ENABLE_DDA}>:dda-grabber>
	# Input
	$<$<BOOL:${ENABLE_FLATBUF_SERVER}>:flatbufserver>
	$<$<BOOL:${ENABLE_PROTOBUF_SERVER}>:protoserver>
	$<$<BOOL:${ENABLE_CEC}>:cechandler>
	# Services
	$<$<BOOL:${ENABLE_EFFECTENGINE} AND TARGET python>:effectengine python>
	$<$<BOOL:${ENABLE_MDNS}>:mdns>
)

#####################################
########### Install steps ###########
#####################################

if(ENABLE_EFFECTENGINE AND TARGET python)
	get_target_property(PYTHON_VERSION python PYTHON_VERSION_PROPERTY)
	get_target_property(PYTHON_MODULES_DIR python PYTHON_STDLIB_LOCATION_PROPERTY)
endif()

# MacOS
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	# set bundle information
	set(MAC_BUNDLE_NAME "Hyperion")
	set_target_properties(${PROJECT_NAME} PROPERTIES
		OUTPUT_NAME ${MAC_BUNDLE_NAME}
		MACOSX_BUNDLE ON
		MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/macos/Info.plist.in"
		MACOSX_BUNDLE_BUNDLE_NAME ${MAC_BUNDLE_NAME}
		MACOSX_BUNDLE_BUNDLE_VERSION ${HYPERION_VERSION}
		MACOSX_BUNDLE_COPYRIGHT "Copyright(c) 2014-2025 Hyperion Project"
		MACOSX_BUNDLE_GUI_IDENTIFIER "com.hyperion-project.${MAC_BUNDLE_NAME}"
		MACOSX_BUNDLE_ICON_FILE "Hyperion.icns"
		MACOSX_BUNDLE_INFO_STRING "${MAC_BUNDLE_NAME} ${HYPERION_VERSION}"
		MACOSX_BUNDLE_SHORT_VERSION_STRING ${HYPERION_VERSION}
		MACOSX_BUNDLE_LONG_VERSION_STRING ${HYPERION_VERSION}
	)

	install(TARGETS ${PROJECT_NAME} BUNDLE DESTINATION . COMPONENT "Hyperion")
	configure_file("${CMAKE_SOURCE_DIR}/cmake/macos/BundleMacOS.cmake.in" "${PROJECT_BINARY_DIR}/BundleMacOS.cmake" @ONLY)
    install(SCRIPT "${PROJECT_BINARY_DIR}/BundleMacOS.cmake" COMPONENT "Hyperion")
endif()

# Linux
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
	# install Hyperion/service files/effect folder
	install(TARGETS ${PROJECT_NAME} DESTINATION "share/hyperion/bin" COMPONENT "Hyperion")
	install(DIRECTORY ${CMAKE_SOURCE_DIR}/bin/service DESTINATION "share/hyperion" COMPONENT "Hyperion")
	install(FILES ${CMAKE_SOURCE_DIR}/effects/readme.txt DESTINATION "share/hyperion/effects" COMPONENT "Hyperion")

	# install Hyperion icons
	set(ICON_SIZES 16 22 24 32 36 48 64 72 96 128 192 256 512)
	foreach(size ${ICON_SIZES})
	  set(ICONS_FROM "${CMAKE_SOURCE_DIR}/resources/icons/hyperion-${size}px.png")
	  set(ICONS_TO "share/hyperion/icons/${size}x${size}/apps/")
	  install(FILES ${ICONS_FROM} DESTINATION ${ICONS_TO} RENAME "hyperion.png" COMPONENT "Hyperion")
	endforeach(size)

	# install desktop/appstream file
	install(FILES ${CMAKE_SOURCE_DIR}/cmake/linux/desktop/hyperion.metainfo.xml DESTINATION "share/hyperion/desktop" COMPONENT "Hyperion")
	install(FILES ${CMAKE_SOURCE_DIR}/cmake/linux/desktop/hyperion.desktop DESTINATION "share/hyperion/desktop" COMPONENT "Hyperion")

	# create symlink and copy "updateHyperionUser.sh" script
	install(CODE "execute_process(COMMAND ln -sf \"../share/hyperion/bin/${PROJECT_NAME}\" \"${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}\")" COMPONENT "Hyperion")
	install(FILES "${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}" DESTINATION "bin" RENAME ${PROJECT_NAME} COMPONENT "Hyperion")
	install(CODE "file (REMOVE ${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME})" COMPONENT "Hyperion")
	install(FILES ${CMAKE_SOURCE_DIR}/bin/scripts/updateHyperionUser.sh DESTINATION "share/hyperion/scripts" COMPONENT "Hyperion")

	if(ENABLE_DEPLOY_DEPENDENCIES)
		configure_file("${CMAKE_SOURCE_DIR}/cmake/linux/BundleLinux.cmake.in" "${PROJECT_BINARY_DIR}/BundleLinux.cmake" @ONLY)
		install(SCRIPT "${PROJECT_BINARY_DIR}/BundleLinux.cmake" COMPONENT "Hyperion")
	endif()
endif()

# Windows
if(WIN32)
	install(TARGETS ${PROJECT_NAME} DESTINATION "bin" COMPONENT "Hyperion" OPTIONAL)
	install(FILES ${CMAKE_SOURCE_DIR}/effects/readme.txt DESTINATION "effects" COMPONENT "Hyperion")

	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP true)
		set(CMAKE_INSTALL_UCRT_LIBRARIES true)
		set(CMAKE_INSTALL_DEBUG_LIBRARIES true)
		set(CMAKE_INSTALL_DEBUG_LIBRARIES_ONLY true)
		include (InstallRequiredSystemLibraries)
	endif()

	if(TARGET Qt${QT_VERSION_MAJOR}::windeployqt)
		get_target_property(WINDEPLOYQT_EXECUTABLE Qt${QT_VERSION_MAJOR}::windeployqt IMPORTED_LOCATION)
	endif()

	if(ENABLE_MF AND TARGET turbojpeg)
		get_target_property(TurboJPEG_INCLUDE_DIR turbojpeg INTERFACE_INCLUDE_DIRECTORIES)
	endif()

	configure_file("${CMAKE_SOURCE_DIR}/cmake/windows/BundleWindows.cmake.in" "${PROJECT_BINARY_DIR}/BundleWindows.cmake" @ONLY)
	install(SCRIPT "${PROJECT_BINARY_DIR}/BundleWindows.cmake" COMPONENT "Hyperion")
endif()

######################################
########## Additional steps ##########
######################################

# Deploy Qt DLLs into the binary folder.
# This is necessary for starting the application from within the IDE
if(WIN32 AND NOT DEFINED ENV{GITHUB_ACTIONS})
	if(TARGET Qt${QT_VERSION_MAJOR}::windeployqt)
		set(WINDEPLOYQT_PARAMS --verbose 0 --no-compiler-runtime --no-opengl-sw --no-system-d3d-compiler)
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND Qt${QT_VERSION_MAJOR}::windeployqt ${WINDEPLOYQT_PARAMS} "$<TARGET_FILE:${PROJECT_NAME}>"
		)
	endif()

	if(OPENSSL_FOUND)
		string(REGEX MATCHALL "[0-9]+" openssl_versions ${OPENSSL_VERSION})
		list(GET openssl_versions 0 openssl_version_major)
		list(GET openssl_versions 1 openssl_version_minor)

		set(open_ssl_version_suffix)
		if(openssl_version_major VERSION_EQUAL 1 AND openssl_version_minor VERSION_EQUAL 1)
			set(open_ssl_version_suffix "-1_1")
		else()
			set(open_ssl_version_suffix "-3")
		endif()

		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
			string(APPEND open_ssl_version_suffix "-x64")
		endif()

		get_filename_component(OPENSSL_DIR "${OPENSSL_INCLUDE_DIR}" DIRECTORY)
		foreach(comp "ssl" "crypto")
			find_file(${comp}
				NAMES
					"lib${comp}${open_ssl_version_suffix}.dll"
				PATHS
					${OPENSSL_DIR}
				PATH_SUFFIXES
					bin
			)

			add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy ${${comp}} $<TARGET_FILE_DIR:${PROJECT_NAME}>
			)
		endforeach()
	endif()
endif()
