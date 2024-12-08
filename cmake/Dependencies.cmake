macro(DeployMacOS TARGET)
	if (EXISTS ${TARGET_FILE})
		message(STATUS "Collecting Dependencies for target file: ${TARGET_FILE}")

		get_target_property(QMAKE_EXECUTABLE Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
		execute_process(
			COMMAND ${QMAKE_EXECUTABLE} -query QT_INSTALL_PLUGINS
			OUTPUT_VARIABLE QT_PLUGIN_DIR
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)

		install(CODE "set(TARGET_FILE \"${TARGET_FILE}\")"					COMPONENT "Hyperion")
		install(CODE "set(TARGET_BUNDLE_NAME \"${TARGET}.app\")"			COMPONENT "Hyperion")
		install(CODE "set(PLUGIN_DIR \"${QT_PLUGIN_DIR}\")"					COMPONENT "Hyperion")
		install(CODE "set(ENABLE_EFFECTENGINE \"${ENABLE_EFFECTENGINE}\")"	COMPONENT "Hyperion")

		install(CODE [[

				set(BUNDLE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/${TARGET_BUNDLE_NAME}")

				file(GET_RUNTIME_DEPENDENCIES
					EXECUTABLES ${TARGET_FILE}
					RESOLVED_DEPENDENCIES_VAR resolved_deps
					UNRESOLVED_DEPENDENCIES_VAR unresolved_deps
				)

				foreach(dependency ${resolved_deps})
					string(FIND ${dependency} "dylib" _index)
					if (${_index} GREATER -1)
						file(INSTALL
							FILES "${dependency}"
							DESTINATION "${BUNDLE_INSTALL_DIR}/Contents/Frameworks"
							TYPE SHARED_LIBRARY
						)
					else()
						file(INSTALL
							FILES "${dependency}"
							DESTINATION "${BUNDLE_INSTALL_DIR}/Contents/lib"
							TYPE SHARED_LIBRARY
							FOLLOW_SYMLINK_CHAIN
						)
					endif()
				endforeach()

				list(LENGTH unresolved_deps unresolved_length)
				if("${unresolved_length}" GREATER 0)
					MESSAGE("The following unresolved dependencies were discovered: ${unresolved_deps}")
				endif()

				foreach(PLUGIN "platforms" "sqldrivers" "imageformats" "tls")
					if(EXISTS ${PLUGIN_DIR}/${PLUGIN})
						file(GLOB files "${PLUGIN_DIR}/${PLUGIN}/*")
						foreach(file ${files})
								file(GET_RUNTIME_DEPENDENCIES
								EXECUTABLES ${file}
								RESOLVED_DEPENDENCIES_VAR PLUGINS
								UNRESOLVED_DEPENDENCIES_VAR unresolved_deps
								)

								foreach(DEPENDENCY ${PLUGINS})
										file(INSTALL
											DESTINATION "${BUNDLE_INSTALL_DIR}/Contents/lib"
											TYPE SHARED_LIBRARY
											FILES ${DEPENDENCY}
											FOLLOW_SYMLINK_CHAIN
										)
								endforeach()

								get_filename_component(singleQtLib ${file} NAME)
								list(APPEND QT_PLUGINS "${BUNDLE_INSTALL_DIR}/Contents/plugins/${PLUGIN}/${singleQtLib}")
								file(INSTALL
									FILES ${file}
									DESTINATION "${BUNDLE_INSTALL_DIR}/Contents/plugins/${PLUGIN}"
									TYPE SHARED_LIBRARY
								)

						endforeach()
					endif()
				endforeach()

				include(BundleUtilities)
				fixup_bundle("${BUNDLE_INSTALL_DIR}" "${QT_PLUGINS}" "${BUNDLE_INSTALL_DIR}/Contents/lib" IGNORE_ITEM "python;python3;Python;Python3;.Python;.Python3")
				file(REMOVE_RECURSE "${BUNDLE_INSTALL_DIR}/Contents/lib")

				if(ENABLE_EFFECTENGINE)
					# Detect the Python version and modules directory
					if(NOT CMAKE_VERSION VERSION_LESS "3.12")
						find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
						set(PYTHON_VERSION_MAJOR_MINOR "${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}")
						set(PYTHON_MODULES_DIR ${Python3_STDLIB})
					else()
						find_package (PythonLibs ${PYTHON_VERSION_STRING} EXACT)
						set(PYTHON_VERSION_MAJOR_MINOR "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")
						set(PYTHON_MODULES_DIR ${Python_STDLIB})
					endif()

					MESSAGE("Add Python ${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR} to bundle")
					MESSAGE("PYTHON_MODULES_DIR: ${PYTHON_MODULES_DIR}")

					# Copy Python modules to '/../Frameworks/Python.framework/Versions/Current/lib/PythonMAJOR.MINOR' and ignore the unnecessary stuff listed below
					if (PYTHON_MODULES_DIR)
						set(PYTHON_FRAMEWORK "${BUNDLE_INSTALL_DIR}/Contents/Frameworks/Python.framework")
						file(
							COPY ${PYTHON_MODULES_DIR}/
							DESTINATION "${PYTHON_FRAMEWORK}/Versions/Current/lib/python${PYTHON_VERSION_MAJOR_MINOR}"
							PATTERN "*.pyc"														EXCLUDE # compiled bytecodes
							PATTERN "__pycache__"												EXCLUDE # any cache
							PATTERN "config-${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}*"	EXCLUDE # static libs
							PATTERN "lib2to3"													EXCLUDE # automated Python 2 to 3 code translation
							PATTERN "tkinter"													EXCLUDE # Tk interface
							PATTERN "lib-dynload/_tkinter.*"									EXCLUDE
							PATTERN "idlelib"													EXCLUDE
							PATTERN "turtle.py"													EXCLUDE # Tk demo
							PATTERN "test"														EXCLUDE # unittest module
							PATTERN "sitecustomize.py"											EXCLUDE # site-specific configs
						)
					endif(PYTHON_MODULES_DIR)
				endif(ENABLE_EFFECTENGINE)

				file(GLOB_RECURSE LIBS FOLLOW_SYMLINKS "${BUNDLE_INSTALL_DIR}/*.dylib")
				file(GLOB FRAMEWORKS FOLLOW_SYMLINKS LIST_DIRECTORIES ON "${BUNDLE_INSTALL_DIR}/Contents/Frameworks/*")
				foreach(item ${LIBS} ${FRAMEWORKS} ${PYTHON_FRAMEWORK} ${BUNDLE_INSTALL_DIR})
					set(cmd codesign --deep --force --sign - "${item}")
					execute_process(
						COMMAND ${cmd}
						RESULT_VARIABLE codesign_result
					)

					if(NOT codesign_result EQUAL 0)
						message(WARNING "macOS signing failed; ${cmd} returned ${codesign_result}")
					endif()
				endforeach()

		]] COMPONENT "Hyperion")

	else()
		add_custom_command(
			TARGET ${TARGET} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" "-DTARGET_FILE=$<TARGET_FILE:${TARGET}>"
			ARGS ${CMAKE_SOURCE_DIR}
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
			VERBATIM
		)
	endif()
endmacro()

macro(DeployLinux TARGET)
	if (EXISTS ${TARGET_FILE})
		message(STATUS "Collecting Dependencies for target file: ${TARGET_FILE}")
		include(GetPrerequisites)

		set(SYSTEM_LIBS_SKIP
			"libatomic"
			"libc"
			"libdbus"
			"libdl"
			"libexpat"
			"libfontconfig"
			"libgcc_s"
			"libgcrypt"
			"libglib"
			"libglib-2"
			"libgpg-error"
			"liblz4"
			"liblzma"
			"libm"
			"libpcre"
			"libpcre2"
			"libpthread"
			"librt"
			"libstdc++"
			"libsystemd"
			"libudev"
			"libusb"
			"libusb-1"
			"libutil"
			"libuuid"
			"libz"
        )

		if (ENABLE_DISPMANX)
			list(APPEND SYSTEM_LIBS_SKIP "libcec")
		endif()

		# Extract dependencies ignoring the system ones
		get_prerequisites(${TARGET_FILE} DEPENDENCIES 0 1 "" "")

		message(STATUS "Dependencies for target file: ${DEPENDENCIES}")

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
		else()
			message( WARNING "OpenSSL NOT found (https webserver will not work)")
		endif(OPENSSL_FOUND)

		# Detect the Qt plugin directory, source: https://github.com/lxde/lxqt-qtplugin/blob/master/src/CMakeLists.txt
		if ( TARGET Qt${QT_VERSION_MAJOR}::qmake )
			get_target_property(QT_QMAKE_EXECUTABLE Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
			execute_process(
				COMMAND ${QT_QMAKE_EXECUTABLE} -query QT_INSTALL_PLUGINS
				OUTPUT_VARIABLE QT_PLUGINS_DIR
				OUTPUT_STRIP_TRAILING_WHITESPACE
			)
		endif()

		# Copy Qt plugins to 'share/hyperion/lib'
		if (QT_PLUGINS_DIR)
			foreach(PLUGIN "platforms" "sqldrivers" "imageformats" "tls" "wayland-shell-integration")
				if (EXISTS ${QT_PLUGINS_DIR}/${PLUGIN})
					file(GLOB files "${QT_PLUGINS_DIR}/${PLUGIN}/*.so")
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

		if(ENABLE_EFFECTENGINE)
			# Detect the Python version and modules directory
			if (NOT CMAKE_VERSION VERSION_LESS "3.12")
				find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
				set(PYTHON_VERSION_MAJOR_MINOR "${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}")
				set(PYTHON_MODULES_DIR ${Python3_STDLIB})
			else()
				find_package (PythonLibs ${PYTHON_VERSION_STRING} EXACT)
				set(PYTHON_VERSION_MAJOR_MINOR "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")
				set(PYTHON_MODULES_DIR ${Python_STDLIB})
			endif()

			# Copy Python modules to 'share/hyperion/lib/pythonMAJOR.MINOR' and ignore the unnecessary stuff listed below
			if (PYTHON_MODULES_DIR)

				install(
					DIRECTORY ${PYTHON_MODULES_DIR}/
					DESTINATION "share/hyperion/lib/python${PYTHON_VERSION_MAJOR_MINOR}"
					COMPONENT "Hyperion"
					PATTERN "*.pyc"                                 EXCLUDE # compiled bytecodes
					PATTERN "__pycache__"                           EXCLUDE # any cache
					PATTERN "config-${PYTHON_VERSION_MAJOR_MINOR}*" EXCLUDE # static libs
					PATTERN "lib2to3"                               EXCLUDE # automated Python 2 to 3 code translation
					PATTERN "tkinter"                               EXCLUDE # Tk interface
					PATTERN "turtle.py"                             EXCLUDE # Tk demo
					PATTERN "test"                                  EXCLUDE # unittest module
					PATTERN "sitecustomize.py"                      EXCLUDE # site-specific configs
				)
			endif(PYTHON_MODULES_DIR)
		endif(ENABLE_EFFECTENGINE)

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
	if (EXISTS ${TARGET_FILE})
		message(STATUS "Collecting Dependencies for target file: ${TARGET_FILE}")
		find_package(Qt${QT_VERSION_MAJOR}Core REQUIRED)
		find_package(OpenSSL REQUIRED)

		# Find the windeployqt binaries
		get_target_property(QMAKE_EXECUTABLE Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
		get_filename_component(QT_BIN_DIR "${QMAKE_EXECUTABLE}" DIRECTORY)
		find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_BIN_DIR}")

		# Collect the runtime libraries
		get_filename_component(COMPILER_PATH "${CMAKE_CXX_COMPILER}" DIRECTORY)
		if (QT_VERSION_MAJOR EQUAL 5)
			set(WINDEPLOYQT_PARAMS --no-angle --no-opengl-sw)
		else()
			set(WINDEPLOYQT_PARAMS --no-opengl-sw)
		endif()

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

		# Copy libssl/libcrypto to 'hyperion'
		if (OPENSSL_FOUND)
			string(REGEX MATCHALL "[0-9]+" openssl_versions "${OPENSSL_VERSION}")
			list(GET openssl_versions 0 openssl_version_major)
			list(GET openssl_versions 1 openssl_version_minor)

			set(open_ssl_version_suffix)
			if (openssl_version_major VERSION_EQUAL 1 AND openssl_version_minor VERSION_EQUAL 1)
				set(open_ssl_version_suffix "-1_1")
			else()
				set(open_ssl_version_suffix "-3")
			endif()

			if (CMAKE_SIZEOF_VOID_P EQUAL 8)
				string(APPEND open_ssl_version_suffix "-x64")
			endif()

			find_file(OPENSSL_SSL
				NAMES "libssl${open_ssl_version_suffix}.dll"
				PATHS ${OPENSSL_INCLUDE_DIR}/.. ${OPENSSL_INCLUDE_DIR}/../bin
				NO_DEFAULT_PATH
			)

			find_file(OPENSSL_CRYPTO
				NAMES "libcrypto${open_ssl_version_suffix}.dll"
				PATHS ${OPENSSL_INCLUDE_DIR}/.. ${OPENSSL_INCLUDE_DIR}/../bin
				NO_DEFAULT_PATH
			)

			install(
				FILES ${OPENSSL_SSL} ${OPENSSL_CRYPTO}
				DESTINATION "bin"
				COMPONENT "Hyperion"
			)
		endif(OPENSSL_FOUND)

		# Copy libjpeg-turbo to 'hyperion'
		if (ENABLE_MF)
			find_package(TurboJPEG)

			if (TURBOJPEG_FOUND)
				find_file(TURBOJPEG_DLL
					NAMES "turbojpeg.dll"
					PATHS ${TurboJPEG_INCLUDE_DIRS}/.. ${TurboJPEG_INCLUDE_DIRS}/../bin
					NO_DEFAULT_PATH
				)

				find_file(JPEG_DLL
					NAMES "jpeg62.dll"
					PATHS ${TurboJPEG_INCLUDE_DIRS}/.. ${TurboJPEG_INCLUDE_DIRS}/../bin
					NO_DEFAULT_PATH
				)

				install(
					FILES ${TURBOJPEG_DLL} ${JPEG_DLL}
					DESTINATION "bin"
					COMPONENT "Hyperion"
				)
			endif(TURBOJPEG_FOUND)
		endif(ENABLE_MF)

		# Create a qt.conf file in 'bin' to override hard-coded search paths in Qt plugins
		file(WRITE "${CMAKE_BINARY_DIR}/qt.conf" "[Paths]\nPlugins=../lib/\n")
		install(
			FILES "${CMAKE_BINARY_DIR}/qt.conf"
			DESTINATION "bin"
			COMPONENT "Hyperion"
		)

		if(ENABLE_EFFECTENGINE)
			# Download embed python package (only release build package available)
			# Currently only cmake version >= 3.12 implemented
			set(url "https://www.python.org/ftp/python/${Python3_VERSION}/")
			set(filename "python-${Python3_VERSION}-embed-amd64.zip")

			if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/${filename}" OR NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/python")
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
		endif(ENABLE_EFFECTENGINE)

		if (ENABLE_DX)
			# Download DirectX End-User Runtimes (June 2010)
			set(url "https://download.microsoft.com/download/8/4/A/84A35BF1-DAFE-4AE8-82AF-AD2AE20B6B14/directx_Jun2010_redist.exe")
			if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/dx_redist.exe")
				file(DOWNLOAD "${url}" "${CMAKE_CURRENT_BINARY_DIR}/dx_redist.exe"
					STATUS result
				)

				# Check if the download is successful
				list(GET result 0 result_code)
				if (NOT result_code EQUAL 0)
					list(GET result 1 reason)
					message(FATAL_ERROR "Could not download DirectX End-User Runtimes: ${reason}")
				endif()
			endif()

			# Copy DirectX End-User Runtimes to 'hyperion'
			install(
				FILES ${CMAKE_CURRENT_BINARY_DIR}/dx_redist.exe
				DESTINATION "bin"
				COMPONENT "Hyperion"
			)
		endif (ENABLE_DX)

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
