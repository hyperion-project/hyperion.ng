macro(DeployUnix TARGET)
	if(EXISTS ${TARGET_FILE})
		message(STATUS "Collecting Dependencies for target file: ${TARGET_FILE}")
		include(GetPrerequisites)

		set(SYSTEM_LIBS_SKIP
			"libc"
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
			"libm"
			"libpthread"
			"librt"
			"libstdc++"
			"libudev"
			"libusb-1"
			"libutil"
			"libX11"
			"libz"
		)

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
							COMPONENT "Hyperion"
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
			COMPONENT "Hyperion"
		)

		# Copy dependencies to 'share/hyperion/lib'
		foreach(PREREQUISITE_LIB ${PREREQUISITE_LIBS})
			install(
				FILES ${PREREQUISITE_LIB}
				DESTINATION "share/hyperion/lib"
				COMPONENT "Hyperion"
			)
		endforeach()

		# Detect the Python version and modules directory
		if (NOT CMAKE_VERSION VERSION_LESS "3.12")

			set(PYTHON_VERSION_MAJOR_MINOR "${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}")
			execute_process(
				COMMAND ${Python3_EXECUTABLE} -c "from distutils.sysconfig import get_python_lib; print(get_python_lib(standard_lib=True))"
				OUTPUT_VARIABLE PYTHON_MODULES_DIR
				OUTPUT_STRIP_TRAILING_WHITESPACE
			)

		else()

			set(PYTHON_VERSION_MAJOR_MINOR "${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR}")
			execute_process(
				COMMAND ${PYTHON_EXECUTABLE} -c "from distutils.sysconfig import get_python_lib; print(get_python_lib(standard_lib=True))"
				OUTPUT_VARIABLE PYTHON_MODULES_DIR
				OUTPUT_STRIP_TRAILING_WHITESPACE
			)

		endif()

		# Pack Python modules to pythonXX.zip or copy to 'share/hyperion/lib/python'
		if (PYTHON_MODULES_DIR)
			# Since version 3.3.2 CMake has the functionality to generate a zip file built-in.
			if (NOT CMAKE_VERSION VERSION_LESS "3.3.2")

				file(GLOB PYTHON_MODULE_FILES RELATIVE "${PYTHON_MODULES_DIR}" "${PYTHON_MODULES_DIR}/*")
				set(PYTHON_ZIP "python${PYTHON_VERSION_MAJOR_MINOR}.zip")

				execute_process(
					COMMAND "${CMAKE_COMMAND}" "-E" "tar" "cf" "${CMAKE_BINARY_DIR}/${PYTHON_ZIP}" "--format=zip" ${PYTHON_MODULE_FILES}
					WORKING_DIRECTORY "${PYTHON_MODULES_DIR}"
					OUTPUT_QUIET
				)

				install(
					FILES "${CMAKE_BINARY_DIR}/${PYTHON_ZIP}"
					DESTINATION "share/hyperion/bin"
					COMPONENT "Hyperion"
				)

			else()

				install(
					DIRECTORY ${PYTHON_MODULES_DIR}/
					DESTINATION "share/hyperion/lib/python"
					COMPONENT "Hyperion"
				)

			endif()

		endif(PYTHON_MODULES_DIR)

	else()
		# Run CMake after target was built to run get_prerequisites on ${TARGET_FILE}
		add_custom_command(
			TARGET ${TARGET} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" "-DTARGET_FILE=$<TARGET_FILE:${TARGET}>"
			ARGS ${CMAKE_SOURCE_DIR}
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
			VERBATIM
		)
	endif()
endmacro()

macro(DeployWindows TARGET)
	if(EXISTS ${TARGET_FILE})
		message(STATUS "Collecting Dependencies for target file: ${TARGET_FILE}")
		find_package(Qt5Core REQUIRED)
		find_package(OpenSSL REQUIRED)

		# Find the windeployqt binaries
		get_target_property(QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
		get_filename_component(QT_BIN_DIR "${QMAKE_EXECUTABLE}" DIRECTORY)
		find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_BIN_DIR}")

		# Collect the runtime libraries
		get_filename_component(COMPILER_PATH "${CMAKE_CXX_COMPILER}" DIRECTORY)
		set(WINDEPLOYQT_PARAMS --no-angle --no-opengl-sw)

		execute_process(
			COMMAND "${CMAKE_COMMAND}" -E
			env "PATH=${COMPILER_PATH};${QT_BIN_DIR}" "${WINDEPLOYQT_EXECUTABLE}"
			--dry-run
			${WINDEPLOYQT_PARAMS}
			--list mapping
			"${TARGET_FILE}"
			OUTPUT_VARIABLE DEPS
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)

		# Parse DEPS into a semicolon-separated list.
		separate_arguments(DEPENDENCIES WINDOWS_COMMAND ${DEPS})
		string(REPLACE "\\" "/" DEPENDENCIES "${DEPENDENCIES}")

		# Copy dependencies to 'hyperion/lib' or 'hyperion'
		while (DEPENDENCIES)
			list(GET DEPENDENCIES 0 src)
			list(GET DEPENDENCIES 1 dst)
			get_filename_component(dst ${dst} DIRECTORY)

			if (NOT "${dst}" STREQUAL "")
				install(
					FILES ${src}
					DESTINATION "lib/${dst}"
					COMPONENT "Hyperion"
				)
			else()
				install(
					FILES ${src}
					DESTINATION "bin"
					COMPONENT "Hyperion"
				)
			endif()

			list(REMOVE_AT DEPENDENCIES 0 1)
		endwhile()

		# Copy OpenSSL Libs
		if (OPENSSL_FOUND)
			string(REGEX MATCHALL "[0-9]+" openssl_versions "${OPENSSL_VERSION}")
			list(GET openssl_versions 0 openssl_version_major)
			list(GET openssl_versions 1 openssl_version_minor)

			set(library_suffix "-${openssl_version_major}_${openssl_version_minor}")
			if(CMAKE_SIZEOF_VOID_P EQUAL 8)
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

			install(
				FILES ${OPENSSL_SSL} ${OPENSSL_CRYPTO}
				DESTINATION "bin"
				COMPONENT "Hyperion"
			)
		endif(OPENSSL_FOUND)

		# Create a qt.conf file in 'bin' to override hard-coded search paths in Qt plugins
		file(WRITE "${CMAKE_BINARY_DIR}/qt.conf" "[Paths]\nPlugins=../lib/\n")
		install(
			FILES "${CMAKE_BINARY_DIR}/qt.conf"
			DESTINATION "bin"
			COMPONENT "Hyperion"
		)

		# Download embed python package
		# Currently only cmake version >= 3.12 implemented
		set(url "https://www.python.org/ftp/python/${Python3_VERSION}/")
		set(filename "python-${Python3_VERSION}-embed-amd64.zip")

		if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/${filename}" OR NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/python")
			file(DOWNLOAD "${url}${filename}" "${CMAKE_CURRENT_BINARY_DIR}/${filename}"
				STATUS result
			)

			# Check if the download is successful
			list(GET result 0 result_code)
			if(NOT result_code EQUAL 0)
				list(GET result 1 reason)
				message(FATAL_ERROR "Could not download file ${url}${filename}: ${reason}")
			endif()

			# Unpack downloaded embed python
			file(REMOVE_RECURSE ${CMAKE_CURRENT_BINARY_DIR}/python)
			file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/python)
			execute_process(
				COMMAND ${CMAKE_COMMAND} -E tar -xfz "${CMAKE_CURRENT_BINARY_DIR}/${filename}"
				WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/python
				OUTPUT_QUIET
			)
		endif()

		# Copy pythonXX.dll and pythonXX.zip to 'hyperion'
		foreach(PYTHON_FILE
			"python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}.dll"
			"python${Python3_VERSION_MAJOR}${Python3_VERSION_MINOR}.zip"
		)
			install(
				FILES ${CMAKE_CURRENT_BINARY_DIR}/python/${PYTHON_FILE}
				DESTINATION "bin"
				COMPONENT "Hyperion"
			)
		endforeach()

	else()
		# Run CMake after target was built
		add_custom_command(
			TARGET ${TARGET} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" "-DTARGET_FILE=$<TARGET_FILE:${TARGET}>"
			ARGS ${CMAKE_SOURCE_DIR}
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
			VERBATIM
		)
	endif()
endmacro()
