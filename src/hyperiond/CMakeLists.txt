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
elseif(CMAKE_SYSTEM MATCHES "Darwin")
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

target_link_libraries(${PROJECT_NAME}
	commandline
	hyperion
	jsonserver
	webserver
	ssdp
	database
	resources
	events
	Qt${QT_VERSION_MAJOR}::Core
	Qt${QT_VERSION_MAJOR}::Gui
	Qt${QT_VERSION_MAJOR}::Network
	Qt${QT_VERSION_MAJOR}::Widgets
)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS DBus QUIET)
if(Qt${QT_VERSION_MAJOR}DBus_FOUND)
	target_link_libraries(${PROJECT_NAME} "Qt${QT_VERSION_MAJOR}::DBus")
endif()

if(ENABLE_EFFECTENGINE AND TARGET python)
	target_link_libraries(${PROJECT_NAME} effectengine python)
	get_target_property(PYTHON_VERSION python PYTHON_VERSION_PROPERTY)
	get_target_property(PYTHON_MODULES_DIR python PYTHON_STDLIB_LOCATION_PROPERTY)
endif()

if(ENABLE_FLATBUF_SERVER)
	target_link_libraries(${PROJECT_NAME} flatbufserver)
endif()

if(ENABLE_PROTOBUF_SERVER)
	target_link_libraries(${PROJECT_NAME} protoserver)
endif()

if(ENABLE_DISPMANX)
	target_link_libraries(${PROJECT_NAME} dispmanx-grabber)
endif()

if(ENABLE_FB)
	target_link_libraries(${PROJECT_NAME} framebuffer-grabber)
endif()

if(ENABLE_OSX)
	target_link_libraries(${PROJECT_NAME} osx-grabber)
endif()

if(ENABLE_V4L2)
	target_link_libraries(${PROJECT_NAME} v4l2-grabber)
endif()

if(ENABLE_MF)
	target_link_libraries(${PROJECT_NAME} mf-grabber)
	if(TARGET turbojpeg)
		get_target_property(TurboJPEG_INCLUDE_DIR turbojpeg INTERFACE_INCLUDE_DIRECTORIES)
	endif()
endif()

if(ENABLE_AUDIO)
	target_link_libraries(hyperiond audio-grabber)
endif()

if(ENABLE_AMLOGIC)
	target_link_libraries(${PROJECT_NAME} amlogic-grabber)
endif()

if(ENABLE_X11)
	if(CMAKE_SYSTEM MATCHES "Darwin")
		include_directories("/opt/X11/include")
	endif()
	target_link_libraries(${PROJECT_NAME} x11-grabber)
endif()

if(ENABLE_XCB)
	target_link_libraries(${PROJECT_NAME} xcb-grabber)
endif()

if(ENABLE_QT)
	target_link_libraries(${PROJECT_NAME} qt-grabber)
endif()

if(ENABLE_DX)
	target_link_libraries(${PROJECT_NAME} directx-grabber)
endif()

if(ENABLE_DDA)
	target_link_libraries(${PROJECT_NAME} dda-grabber)
endif()

if(ENABLE_CEC)
	target_link_libraries(${PROJECT_NAME} cechandler)
endif()

if(ENABLE_MDNS)
	target_link_libraries(${PROJECT_NAME} mdns)
endif()

#####################################
########### Install steps ###########
#####################################

if(CMAKE_SYSTEM MATCHES "Darwin")
	# set bundle information
	set(MAC_BUNDLE_NAME "Hyperion")
	set_target_properties(${PROJECT_NAME} PROPERTIES
		OUTPUT_NAME ${MAC_BUNDLE_NAME}
		MACOSX_BUNDLE ON
		MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/macos/Info.plist.in"
		MACOSX_BUNDLE_BUNDLE_NAME ${MAC_BUNDLE_NAME}
		MACOSX_BUNDLE_BUNDLE_VERSION ${HYPERION_VERSION}
		MACOSX_BUNDLE_COPYRIGHT "Copyright(c) 2014-2024 Hyperion Project"
		MACOSX_BUNDLE_GUI_IDENTIFIER "com.hyperion-project.${MAC_BUNDLE_NAME}"
		MACOSX_BUNDLE_ICON_FILE "Hyperion.icns"
		MACOSX_BUNDLE_INFO_STRING "${MAC_BUNDLE_NAME} ${HYPERION_VERSION}"
		MACOSX_BUNDLE_SHORT_VERSION_STRING ${HYPERION_VERSION}
		MACOSX_BUNDLE_LONG_VERSION_STRING ${HYPERION_VERSION}
	)

	install(TARGETS ${PROJECT_NAME} BUNDLE DESTINATION . COMPONENT "Hyperion")
	configure_file("${CMAKE_SOURCE_DIR}/cmake/macos/BundleMacOS.cmake.in" "${PROJECT_BINARY_DIR}/BundleMacOS.cmake" @ONLY)
    install(SCRIPT "${PROJECT_BINARY_DIR}/BundleMacOS.cmake" COMPONENT "Hyperion")
elseif(NOT WIN32)
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
else()
	install(TARGETS ${PROJECT_NAME} DESTINATION "bin" COMPONENT "Hyperion" OPTIONAL)
	install(FILES ${CMAKE_SOURCE_DIR}/effects/readme.txt DESTINATION "effects" COMPONENT "Hyperion")

	if(TARGET Qt${QT_VERSION_MAJOR}::windeployqt)
		get_target_property(WINDEPLOYQT_EXECUTABLE Qt${QT_VERSION_MAJOR}::windeployqt IMPORTED_LOCATION)
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
