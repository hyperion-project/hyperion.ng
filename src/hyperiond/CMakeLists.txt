find_package(PythonLibs 3.4 REQUIRED)
find_package(Qt5Widgets REQUIRED)
include_directories(${PYTHON_INCLUDE_DIRS} ${PYTHON_INCLUDE_DIRS}/..)

macro(InstallDependencies TARGET INSTALL_COMPONENT)
	set(TARGET_FILE ${CMAKE_BINARY_DIR}/bin/${TARGET})
	set(SYSTEM_LIBS_SKIP
#		"libbsd"
		"libc"
#		"libdbus-1"
		"libdl"
		"libexpat"
		"libfontconfig"
		"libfreetype"
		"libgcc_s"
		"libgcrypt"
		"libGL"
		"libGLdispatch"
		"libglib-2"
		"libGLX"
		"libgpg-error"
#		"liblz4"
#		"liblzma"
		"libm"
		"libpthread"
		"librt"
		"libstdc++"
#		"libsystemd"
		"libudev"
		"libusb-1"
		"libutil"
		"libX11"
#		"libXau"
#		"libxcb"
#		"libXdmcp"
#		"libXext"
#		"libXrender"
		"libz"
	)

	if(EXISTS ${TARGET_FILE})
		include(GetPrerequisites)

		if (APPLE)
			set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
		endif(APPLE)

		# Extract dependencies ignoring the system ones
		get_prerequisites(${TARGET_FILE} DEPENDENCIES 0 1 "" "")

		# Append symlink and non-symlink dependencies to the list
		set(PREREQUISITE_LIBS "")
		foreach(DEPENDENCY ${DEPENDENCIES})
			get_filename_component(resolved ${DEPENDENCY} NAME_WE)
			list(FIND SYSTEM_LIBS_SKIP ${resolved} _index)
			if (${_index} GREATER -1)
				continue() # Skip system libraries
			else()
				gp_resolve_item("${TARGET_FILE}" "${DEPENDENCY}" "" "" resolved_file)
				get_filename_component(resolved_file ${resolved_file} ABSOLUTE)
				gp_append_unique(PREREQUISITE_LIBS ${resolved_file})
				get_filename_component(file_canonical ${resolved_file} REALPATH)
				gp_append_unique(PREREQUISITE_LIBS ${file_canonical})
			endif()
		endforeach()

		# Append the OpenSSL library to the list
		find_package(OpenSSL 1.0.0 REQUIRED)
		if (OPENSSL_FOUND)
			foreach(openssl_lib ${OPENSSL_LIBRARIES})
				get_prerequisites(${openssl_lib} openssl_deps 0 1 "" "")

				foreach(openssl_dep ${openssl_deps})
					get_filename_component(resolved ${openssl_dep} NAME_WE)
					list(FIND SYSTEM_LIBS_SKIP ${resolved} _index)
					if (${_index} GREATER -1)
						continue() # Skip system libraries
					else()
						gp_resolve_item("${openssl_lib}" "${openssl_dep}" "" "" resolved_file)
						get_filename_component(resolved_file ${resolved_file} ABSOLUTE)
						gp_append_unique(PREREQUISITE_LIBS ${resolved_file})
						get_filename_component(file_canonical ${resolved_file} REALPATH)
						gp_append_unique(PREREQUISITE_LIBS ${file_canonical})
					endif()
				endforeach()

				gp_append_unique(PREREQUISITE_LIBS ${openssl_lib})
				get_filename_component(file_canonical ${openssl_lib} REALPATH)
				gp_append_unique(PREREQUISITE_LIBS ${file_canonical})
			endforeach()
		endif(OPENSSL_FOUND)

		# Detect the Qt5 plugin directory, source: https://github.com/lxde/lxqt-qtplugin/blob/master/src/CMakeLists.txt
		get_target_property(QT_QMAKE_EXECUTABLE ${Qt5Core_QMAKE_EXECUTABLE} IMPORTED_LOCATION)
		execute_process(
			COMMAND ${QT_QMAKE_EXECUTABLE} -query QT_INSTALL_PLUGINS
			OUTPUT_VARIABLE QT_PLUGINS_DIR
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)

		# Copy Qt plugins to 'share/hyperion/lib'
		if(QT_PLUGINS_DIR)
			foreach(PLUGIN "platforms" "sqldrivers" "imageformats")
				if(EXISTS ${QT_PLUGINS_DIR}/${PLUGIN})
					file(GLOB files "${QT_PLUGINS_DIR}/${PLUGIN}/*")
					foreach(file ${files})
						get_prerequisites(${file} PLUGINS 0 1 "" "")

						foreach(DEPENDENCY ${PLUGINS})
							get_filename_component(resolved ${DEPENDENCY} NAME_WE)
							list(FIND SYSTEM_LIBS_SKIP ${resolved} _index)
							if (${_index} GREATER -1)
								continue() # Skip system libraries
							else()
								gp_resolve_item("${file}" "${DEPENDENCY}" "" "" resolved_file)
								get_filename_component(resolved_file ${resolved_file} ABSOLUTE)
								gp_append_unique(PREREQUISITE_LIBS ${resolved_file})
								get_filename_component(file_canonical ${resolved_file} REALPATH)
								gp_append_unique(PREREQUISITE_LIBS ${file_canonical})
							endif()
						endforeach()

						install(
							FILES ${file}
							DESTINATION "share/hyperion/lib/${PLUGIN}"
							COMPONENT "${INSTALL_COMPONENT}"
						)
					endforeach()
				endif()
			endforeach()
		endif(QT_PLUGINS_DIR)

		# Create a qt.conf file in 'share/hyperion/bin' to override hard-coded search paths in Qt plugins
		file(WRITE "${CMAKE_BINARY_DIR}/qt.conf" "[Paths]\nPlugins=../lib/\n")
		install(
			FILES "${CMAKE_BINARY_DIR}/qt.conf"
			DESTINATION "share/hyperion/bin"
			COMPONENT "${INSTALL_COMPONENT}"
		)

		# Copy dependencies to 'share/hyperion/lib'
		foreach(PREREQUISITE_LIB ${PREREQUISITE_LIBS})
			install(
				FILES ${PREREQUISITE_LIB}
				DESTINATION "share/hyperion/lib"
				COMPONENT "${INSTALL_COMPONENT}"
			)
		endforeach()

		# Detect the Python modules directory
		execute_process(
			COMMAND ${PYTHON_EXECUTABLE} -c "from distutils.sysconfig import get_python_lib; print(get_python_lib(standard_lib=True))"
			OUTPUT_VARIABLE PYTHON_MODULES_DIR
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)

		# Copy Python modules to 'share/hyperion/lib/python'
		if (PYTHON_MODULES_DIR)
			install(
				DIRECTORY ${PYTHON_MODULES_DIR}/
				DESTINATION "share/hyperion/lib/python"
				COMPONENT "${INSTALL_COMPONENT}"
			)
		endif(PYTHON_MODULES_DIR)
	else()
		# Run CMake after target was built to run get_prerequisites on ${TARGET_FILE}
		add_custom_command(
			TARGET ${TARGET} POST_BUILD
			COMMAND ${CMAKE_COMMAND}
			ARGS ${CMAKE_SOURCE_DIR}
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
			VERBATIM
		)
	endif()
endmacro()

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
		ssdp
		database
		python
		resources
		${PYTHON_LIBRARIES}
		Qt5::Widgets
)

if (ENABLE_AVAHI)
	target_link_libraries(hyperiond bonjour)
endif ()

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


# Copy dependencies (not for OSX)
if (NOT ENABLE_OSX)
	InstallDependencies("hyperiond" ${PLATFORM})
endif ()
