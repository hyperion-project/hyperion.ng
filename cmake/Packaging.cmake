# cmake file for generating distribution packages

include_guard(GLOBAL)

# Default packages to build
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	set(CPACK_GENERATOR "DragNDrop")
	set(CPACK_DMG_FORMAT "UDBZ")
	set(CMAKE_SYSTEM_NAME "macOS")
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
	set(CPACK_GENERATOR "TGZ")
elseif(WIN32)
	set(CPACK_GENERATOR "ZIP" "External")
	# Overwrite CMAKE_SYSTEM_PROCESSOR for Windows (visual)
	if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "AMD64")
		set(CMAKE_SYSTEM_PROCESSOR "x64")
	elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "ARM64")
		set(CMAKE_SYSTEM_PROCESSOR "arm64")
	endif()
endif()

# Determine packages by found generator executables
find_program(RPM_BUILDER rpm)
if(RPM_BUILDER)
	message(STATUS "CPACK: Found RPM builder")
	set(CPACK_GENERATOR ${CPACK_GENERATOR} "RPM")
endif()

find_program(DEB_BUILDER dpkg-deb)
if(DEB_BUILDER)
	message(STATUS "CPACK: Found DEB builder")
	set(CPACK_GENERATOR ${CPACK_GENERATOR} "DEB")
endif()

# Parameter CPACK_SYSTEM_PROCESSOR overwrites CMAKE_SYSTEM_PROCESSOR
if(CPACK_SYSTEM_PROCESSOR)
	set(CMAKE_SYSTEM_PROCESSOR ${CPACK_SYSTEM_PROCESSOR})
endif()

# Apply to all packages, some of these can be overwritten with generator specific content
# https://cmake.org/cmake/help/v3.5/module/CPack.html

set(CPACK_PACKAGE_NAME "Hyperion")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Hyperion is an open source ambient light implementation")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")

if(NOT CMAKE_VERSION VERSION_LESS "3.18")
	set(CPACK_ARCHIVE_THREADS 0)
endif()

# Replease "+", as cmake/rpm has an issue if"+" occurs in CPACK_PACKAGE_VERSION
string(REPLACE "+" "." HYPERION_PACKAGE_VERSION ${HYPERION_VERSION})
set(CPACK_PACKAGE_FILE_NAME "Hyperion-${HYPERION_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

set(CPACK_PACKAGE_CONTACT "admin@hyperion-project.org")
set(CPACK_PACKAGE_VENDOR "hyperion-project")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://hyperion-project.org")
set(CPACK_PACKAGE_EXECUTABLES "hyperiond;Hyperion")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Hyperion")
set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/resources/icons/hyperion-32px.png")

set(CPACK_PACKAGE_VERSION_MAJOR "${HYPERION_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${HYPERION_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${HYPERION_VERSION_PATCH}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_EXECUTABLES "hyperiond;Hyperion")
set(CPACK_CREATE_DESKTOP_LINKS "hyperiond;Hyperion")

# Append pre-release version to CPACK_PACKAGE_VERSION_PATCH ifexists
if(NOT "${HYPERION_VERSION_PRE}" STREQUAL "")
	string(APPEND CPACK_PACKAGE_VERSION_PATCH ${HYPERION_VERSION_PRE})
endif()

# Specific CPack Package Generators
# https://gitlab.kitware.com/cmake/community/-/wikis/doc/cpack/PackageGenerators

# DEB (UNIX only)
# https://cmake.org/cmake/help/latest/cpack_gen/deb.html
if(CPACK_GENERATOR MATCHES ".*DEB.*")
	set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_SOURCE_DIR}/cmake/linux/package-scripts/preinst;${CMAKE_SOURCE_DIR}/cmake/linux/package-scripts/postinst;${CMAKE_SOURCE_DIR}/cmake/linux/package-scripts/prerm")
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "libcec6 | libcec4 | libcec (>= 4.0)")
	set(CPACK_DEBIAN_PACKAGE_SECTION "Miscellaneous")
endif()

# RPM (Unix Only)
# https://cmake.org/cmake/help/latest/cpack_gen/rpm.html
if(CPACK_GENERATOR MATCHES ".*RPM.*")
	set(CPACK_RPM_PACKAGE_RELEASE 1)
	set(CPACK_RPM_PACKAGE_LICENSE "MIT")
	set(CPACK_RPM_PACKAGE_GROUP "Applications")
	set(CPACK_RPM_PACKAGE_REQUIRES "libcec >= 4.0.0")
	set(CPACK_RPM_PRE_INSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/cmake/linux/package-scripts/preinst")
	set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/cmake/linux/package-scripts/postinst")
	set(CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/cmake/linux/package-scripts/prerm")
endif()

# DragNDrop (macOS only)
# https://cmake.org/cmake/help/latest/cpack_gen/dmg.html
if(CPACK_GENERATOR MATCHES "DragNDrop")
	set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/cmake/macos/PackageIcon.icns")
	set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_BINARY_DIR}/LICENSE")
	set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_SOURCE_DIR}/cmake/macos/Background.png")
	set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${CMAKE_SOURCE_DIR}/cmake/macos/AppleScript.scpt")
	set(CPACK_COMMAND_HDIUTIL "${CMAKE_SOURCE_DIR}/cmake/macos/hdiutilwrap.sh")
endif()

# NSIS (Windows only)
if(WIN32)
	set(CPACK_EXTERNAL_PACKAGE_SCRIPT ${CMAKE_SOURCE_DIR}/cmake/windows/inno/innosetup.cmake)
	set(CPACK_EXTERNAL_ENABLE_STAGING ON)
	set(CPACK_BUILD_CONFIG ${CMAKE_BUILD_TYPE})
	set(CPACK_SYSTEM_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR})
endif()

# Define the install components
# See also https://gitlab.kitware.com/cmake/community/-/wikis/doc/cpack/Component-Install-With-CPack
# and https://cmake.org/cmake/help/latest/module/CPackComponent.html
set(CPACK_COMPONENTS_GROUPING "ALL_COMPONENTS_IN_ONE")

# Components base (All builds)
set(CPACK_COMPONENTS_ALL "Hyperion")

# Optional compiled

if(ENABLE_REMOTE_CTL)
	list(APPEND CPACK_COMPONENTS_ALL "hyperion_remote")
endif()

# only include standalone grabber with build was with flatbuffer client
if(ENABLE_FLATBUF_CONNECT)
	if(ENABLE_QT)
		list(APPEND CPACK_COMPONENTS_ALL "hyperion_qt")
	endif()
	if(ENABLE_AMLOGIC)
		list(APPEND CPACK_COMPONENTS_ALL "hyperion_aml")
	endif()
	if(ENABLE_V4L2)
		list(APPEND CPACK_COMPONENTS_ALL "hyperion_v4l2")
	endif()
	if(ENABLE_AUDIO)
		list(APPEND CPACK_COMPONENTS_ALL "hyperion_audio")
	endif()
	if(ENABLE_X11)
		list(APPEND CPACK_COMPONENTS_ALL "hyperion_x11")
	endif()
	if(ENABLE_XCB)
		list(APPEND CPACK_COMPONENTS_ALL "hyperion_xcb")
	endif()
	if(ENABLE_DISPMANX)
		list(APPEND CPACK_COMPONENTS_ALL "hyperion_dispmanx")
	endif()
	if(ENABLE_FB)
		list(APPEND CPACK_COMPONENTS_ALL "hyperion_framebuffer")
	endif()
	if(ENABLE_OSX)
		list(APPEND CPACK_COMPONENTS_ALL "hyperion_osx")
	endif()
endif()

if(ENABLE_DEPLOY_DEPENDENCIES)
	list(APPEND CPACK_COMPONENTS_ALL "dependencies")
endif()

# Only include Hyperion to macOS dmg package (without standalone programs)
if(CPACK_GENERATOR MATCHES "DragNDrop")
	list(REMOVE_ITEM CPACK_COMPONENTS_ALL "hyperion_remote" "hyperion_qt" "hyperion_osx")
endif()

set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_RPM_COMPONENT_INSTALL ON)

set(CPACK_STRIP_FILES ON)

# no code after following line!
include(CPack)
