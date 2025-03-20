# cmake file for generating distribution packages

# Default packages to build
if(CMAKE_SYSTEM MATCHES "Darwin")
	set(CPACK_GENERATOR "DragNDrop")
	set(CPACK_DMG_FORMAT "UDBZ" )
	set(CMAKE_SYSTEM_NAME "macOS")
elseif(CMAKE_SYSTEM MATCHES "Linux")
	set(CPACK_GENERATOR "TGZ")
elseif(WIN32)
	set(CPACK_GENERATOR "ZIP" "NSIS")
	# Overwrite CMAKE_SYSTEM_PROCESSOR for Windows (visual)
	if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "AMD64")
		set(CMAKE_SYSTEM_PROCESSOR "x64")
	endif()
endif()

# Determine packages by found generator executables
find_package(RpmBuilder)
if(RPM_BUILDER_FOUND)
	message(STATUS "CPACK: Found RPM builder")
	set(CPACK_GENERATOR ${CPACK_GENERATOR} "RPM")
endif()

find_package(DebBuilder)
if(DEB_BUILDER_FOUND)
	message(STATUS "CPACK: Found DEB builder")
	set(CPACK_GENERATOR ${CPACK_GENERATOR} "DEB")
endif()

# Parameter CPACK_SYSTEM_PROCESSOR overwrites CMAKE_SYSTEM_PROCESSOR
if(CPACK_SYSTEM_PROCESSOR)
	set(CMAKE_SYSTEM_PROCESSOR ${CPACK_SYSTEM_PROCESSOR})
endif()

# Apply to all packages, some of these can be overwritten with generator specific content
# https://cmake.org/cmake/help/v3.5/module/CPack.html

set(CPACK_PACKAGE_NAME "Hyperion" )
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Hyperion is an open source ambient light implementation" )
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md" )

if(NOT CMAKE_VERSION VERSION_LESS "3.18")
	set(CPACK_ARCHIVE_THREADS 0)
endif()

# Replease "+", as cmake/rpm has an issue if"+" occurs in CPACK_PACKAGE_VERSION
string(REPLACE "+" "." HYPERION_PACKAGE_VERSION ${HYPERION_VERSION})
set(CPACK_PACKAGE_FILE_NAME "Hyperion-${HYPERION_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

set(CPACK_PACKAGE_CONTACT "packages@hyperion-project.org")
set(CPACK_PACKAGE_VENDOR "hyperion-project")
set(CPACK_PACKAGE_EXECUTABLES "hyperiond;Hyperion" )
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Hyperion" )
set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/resources/icons/hyperion-32px.png" )

set(CPACK_PACKAGE_VERSION_MAJOR "${HYPERION_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${HYPERION_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${HYPERION_VERSION_PATCH}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE" )
set(CPACK_PACKAGE_EXECUTABLES "hyperiond;Hyperion" )
set(CPACK_CREATE_DESKTOP_LINKS "hyperiond;Hyperion" )

# Append pre-release version to CPACK_PACKAGE_VERSION_PATCH ifexists
if(NOT "${HYPERION_VERSION_PRE}" STREQUAL "")
	string(APPEND CPACK_PACKAGE_VERSION_PATCH ${HYPERION_VERSION_PRE})
endif()

# Specific CPack Package Generators
# https://gitlab.kitware.com/cmake/community/-/wikis/doc/cpack/PackageGenerators

# DEB (UNIX only)
# https://cmake.org/cmake/help/latest/cpack_gen/deb.html
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_SOURCE_DIR}/cmake/linux/package-scripts/preinst;${CMAKE_SOURCE_DIR}/cmake/linux/package-scripts/postinst;${CMAKE_SOURCE_DIR}/cmake/linux/package-scripts/prerm")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libcec6 | libcec4 | libcec (>= 4.0)" )
set(CPACK_DEBIAN_PACKAGE_SECTION "Miscellaneous" )

# RPM (Unix Only)
# https://cmake.org/cmake/help/latest/cpack_gen/rpm.html
set(CPACK_RPM_PACKAGE_RELEASE 1 )
set(CPACK_RPM_PACKAGE_LICENSE "MIT" )
set(CPACK_RPM_PACKAGE_GROUP "Applications" )
set(CPACK_RPM_PACKAGE_REQUIRES "libcec >= 4.0.0" )
set(CPACK_RPM_PRE_INSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/cmake/linux/package-scripts/preinst")
set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/cmake/linux/package-scripts/postinst")
set(CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/cmake/linux/package-scripts/prerm")

# DragNDrop (macOS only)
# https://cmake.org/cmake/help/latest/cpack_gen/dmg.html
if(CMAKE_SYSTEM MATCHES "Darwin")
	set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/cmake/macos/PackageIcon.icns" )
	set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_BINARY_DIR}/LICENSE" )
	set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_SOURCE_DIR}/cmake/macos/Background.png" )
	set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${CMAKE_SOURCE_DIR}/cmake/macos/AppleScript.scpt" )
endif()

# NSIS (Windows only)
# https://gitlab.kitware.com/cmake/community/-/wikis/doc/cpack/PackageGenerators#nsis
if(WIN32)
	# Use custom script based on cpack nsis template
	set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/nsis/template ${CMAKE_MODULE_PATH})

	# Some path transformations
	file(TO_NATIVE_PATH ${CPACK_PACKAGE_ICON} CPACK_PACKAGE_ICON)
	string(REGEX REPLACE "\\\\" "\\\\\\\\" CPACK_PACKAGE_ICON ${CPACK_PACKAGE_ICON})

	file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}/cmake/windows/nsis/installer.ico" NSIS_HYP_ICO)
	file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}/cmake/windows/nsis/header.bmp" NSIS_HYP_LOGO_HORI)
	file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}/cmake/windows/nsis/logo.bmp" NSIS_HYP_LOGO_VERT)

	string(REGEX REPLACE "\\\\" "\\\\\\\\" NSIS_HYP_ICO "${NSIS_HYP_ICO}")
	string(REGEX REPLACE "\\\\" "\\\\\\\\" NSIS_HYP_LOGO_VERT "${NSIS_HYP_LOGO_VERT}")
	string(REGEX REPLACE "\\\\" "\\\\\\\\" NSIS_HYP_LOGO_HORI "${NSIS_HYP_LOGO_HORI}")

	set(CPACK_NSIS_MODIFY_PATH ON )
	set(CPACK_NSIS_MUI_ICON ${NSIS_HYP_ICO})
	set(CPACK_NSIS_MUI_UNIICON ${NSIS_HYP_ICO})
	set(CPACK_NSIS_MUI_HEADERIMAGE ${NSIS_HYP_LOGO_HORI} )
	set(CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP ${NSIS_HYP_LOGO_VERT})
	set(CPACK_NSIS_DISPLAY_NAME "Hyperion Ambient Light")
	set(CPACK_NSIS_PACKAGE_NAME "Hyperion" )
	set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\hyperiond.exe")
	set(CPACK_NSIS_HELP_LINK "https://www.hyperion-project.org")
	set(CPACK_NSIS_URL_INFO_ABOUT "https://www.hyperion-project.org")
	set(CPACK_NSIS_MUI_FINISHPAGE_RUN "hyperiond.exe")
	set(CPACK_NSIS_BRANDING_TEXT "Hyperion-${HYPERION_VERSION}")

	# custom nsis plugin directory
	set(CPACK_NSIS_EXTRA_DEFS "!addplugindir ${CMAKE_SOURCE_DIR}/cmake/windows/nsis/plugins")

	# additional hyperiond startmenu link, won't be created ifthe user disables startmenu links
	set(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Hyperion (Console).lnk' '$INSTDIR\\\\bin\\\\hyperiond.exe' '-d -c'")
	set(CPACK_NSIS_DELETE_ICONS_EXTRA "Delete '$SMPROGRAMS\\\\$MUI_TEMP\\\\Hyperion (Console).lnk'")
endif()

# Define the install components
# See also https://gitlab.kitware.com/cmake/community/-/wikis/doc/cpack/Component-Install-With-CPack
# and https://cmake.org/cmake/help/latest/module/CPackComponent.html
set(CPACK_COMPONENTS_GROUPING "ALL_COMPONENTS_IN_ONE")

# Components base (All builds)
set(CPACK_COMPONENTS_ALL "Hyperion" )

# Optional compiled

if(ENABLE_REMOTE_CTL)
	set(CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} "hyperion_remote" )
endif()

# only include standalone grabber with build was with flatbuffer client
if(ENABLE_FLATBUF_CONNECT)
	if(ENABLE_QT)
		set(CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} "hyperion_qt" )
	endif()
	if(ENABLE_AMLOGIC)
		set(CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} "hyperion_aml" )
	endif()
	if(ENABLE_V4L2)
		set(CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} "hyperion_v4l2" )
	endif()
	if(ENABLE_AUDIO)
		set(CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} "hyperion_audio" )
	endif()
	if(ENABLE_X11)
		set(CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} "hyperion_x11" )
	endif()
	if(ENABLE_XCB)
		set(CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} "hyperion_xcb" )
	endif()
	if(ENABLE_DISPMANX)
		set(CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} "hyperion_dispmanx" )
	endif()
	if(ENABLE_FB)
		set(CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} "hyperion_framebuffer" )
	endif()
	if(ENABLE_OSX)
		set(CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} "hyperion_osx" )
	endif()
endif(ENABLE_FLATBUF_CONNECT)

# Only include Hyperion to macOS dmg package (without standalone programs)
if(CPACK_GENERATOR MATCHES "DragNDrop" )
	LIST ( REMOVE_ITEM CPACK_COMPONENTS_ALL "hyperion_remote" "hyperion_qt" "hyperion_osx" )
endif()

set(CPACK_ARCHIVE_COMPONENT_INSTALL ON )
set(CPACK_DEB_COMPONENT_INSTALL ON )
set(CPACK_RPM_COMPONENT_INSTALL ON )

set(CPACK_STRIP_FILES ON )

# no code after following line!
include(CPack)

cpack_add_install_type(Full DISPLAY_NAME "Full")
cpack_add_install_type(Min DISPLAY_NAME "Minimal")
cpack_add_component_group(Runtime EXPANDED DESCRIPTION "Hyperion runtime")

# Components base
cpack_add_component(Hyperion
	DISPLAY_NAME "Hyperion"
	DESCRIPTION "Hyperion runtime"
	INSTALL_TYPES Full Min
	GROUP Runtime
	REQUIRED
)

# optional components

if(ENABLE_REMOTE_CTL)
cpack_add_component_group(Remote DESCRIPTION "hyperion-remote commandline tool")
cpack_add_component(hyperion_remote
	DISPLAY_NAME "Hyperion Remote"
	DESCRIPTION "Hyperion remote cli tool"
	INSTALL_TYPES Full
	GROUP Remote
	DEPENDS Hyperion
)
endif()

# only include standalone grabber with build was with flatbuffer client
if(ENABLE_FLATBUF_CONNECT)
	cpack_add_component_group(Screencapture EXPANDED DESCRIPTION "Standalone Screencapture commandline programs")
	if(ENABLE_QT)
		cpack_add_component(hyperion_qt
			DISPLAY_NAME "Qt Standalone Screencap"
			DESCRIPTION "Qt based standalone screen capture"
			INSTALL_TYPES Full
			GROUP Screencapture
			DEPENDS Hyperion
		)
	endif()
	if(ENABLE_AMLOGIC)
		cpack_add_component(hyperion_aml
			DISPLAY_NAME "Amlogic Standalone Screencap"
			DESCRIPTION "Amlogic based standalone screen capture"
			INSTALL_TYPES Full
			GROUP Screencapture
			DEPENDS Hyperion
		)
	endif()
	if(ENABLE_V4L2)
		cpack_add_component(hyperion_v4l2
			DISPLAY_NAME "V4l2 Standalone Screencap"
			DESCRIPTION "Video for Linux 2 based standalone screen capture"
			INSTALL_TYPES Full
			GROUP Screencapture
			DEPENDS Hyperion
		)
	endif()
	if(ENABLE_X11)
		cpack_add_component(hyperion_x11
			DISPLAY_NAME "X11 Standalone Screencap"
			DESCRIPTION "X11 based standalone screen capture"
			INSTALL_TYPES Full
			GROUP Screencapture
			DEPENDS Hyperion
		)
	endif()
	if(ENABLE_XCB)
		cpack_add_component(hyperion_xcb
			DISPLAY_NAME "XCB Standalone Screencap"
			DESCRIPTION "XCB based standalone screen capture"
			INSTALL_TYPES Full
			GROUP Screencapture
			DEPENDS Hyperion
		)
	endif()
	if(ENABLE_DISPMANX)
		cpack_add_component(hyperion_dispmanx
			DISPLAY_NAME "RPi dispmanx Standalone Screencap"
			DESCRIPTION "Raspbery Pi dispmanx based standalone screen capture"
			INSTALL_TYPES Full
			GROUP Screencapture
			DEPENDS Hyperion
		)
	endif()
	if(ENABLE_FB)
		cpack_add_component(hyperion_framebuffer
			DISPLAY_NAME "Framebuffer Standalone Screencap"
			DESCRIPTION "Framebuffer based standalone screen capture"
			INSTALL_TYPES Full
			GROUP Screencapture
			DEPENDS Hyperion
		)
	endif()
	if(ENABLE_OSX)
		cpack_add_component(hyperion_osx
			DISPLAY_NAME "Mac osx Standalone Screencap"
			DESCRIPTION "Mac osx based standalone screen capture"
			INSTALL_TYPES Full
			GROUP Screencapture
			DEPENDS Hyperion
		)
	endif()
endif(ENABLE_FLATBUF_CONNECT)
