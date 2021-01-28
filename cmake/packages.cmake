# cmake file for generating distribution packages

# Default packages to build
IF (APPLE)
	SET ( CPACK_GENERATOR "DragNDrop")
	SET ( CPACK_DMG_FORMAT "UDBZ" )
ELSEIF (UNIX)
	SET ( CPACK_GENERATOR "TGZ")
ELSEIF (WIN32)
	SET ( CPACK_GENERATOR "ZIP" "NSIS")
ENDIF()

# Determine packages by found generator executables
find_package(RpmBuilder)
IF(RPM_BUILDER_FOUND)
	message(STATUS "CPACK: Found RPM builder")
	SET ( CPACK_GENERATOR ${CPACK_GENERATOR} "RPM")
ENDIF()

find_package(DebBuilder)
IF(DEB_BUILDER_FOUND)
	message(STATUS "CPACK: Found DEB builder")
	SET ( CPACK_GENERATOR ${CPACK_GENERATOR} "DEB")
ENDIF()

# Overwrite CMAKE_SYSTEM_NAME for mac (visual)
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    if(${CMAKE_HOST_APPLE})
        set(CMAKE_SYSTEM_NAME "macOS")
    endif()
endif()

# Overwrite CMAKE_SYSTEM_PROCESSOR for Windows (visual)
if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "AMD64")
    if(${CMAKE_HOST_WIN32})
        set(CMAKE_SYSTEM_PROCESSOR "x64")
    endif()
endif()

# Apply to all packages, some of these can be overwritten with generator specific content
# https://cmake.org/cmake/help/v3.5/module/CPack.html

SET ( CPACK_PACKAGE_NAME "Hyperion" )
SET ( CPACK_PACKAGE_DESCRIPTION_SUMMARY "Hyperion is an open source ambient light implementation" )
SET ( CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md" )

# Replease "+", as cmake/rpm has an issue if "+" occurs in CPACK_PACKAGE_VERSION
string(REPLACE "+" "." HYPERION_PACKAGE_VERSION ${HYPERION_VERSION})
SET ( CPACK_PACKAGE_FILE_NAME "Hyperion-${HYPERION_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

SET ( CPACK_PACKAGE_CONTACT "packages@hyperion-project.org")
SET ( CPACK_PACKAGE_VENDOR "hyperion-project")
SET ( CPACK_PACKAGE_EXECUTABLES "hyperiond;Hyperion" )
SET ( CPACK_PACKAGE_INSTALL_DIRECTORY "Hyperion" )
SET ( CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/resources/icons/hyperion-icon-32px.png" )

SET ( CPACK_PACKAGE_VERSION_MAJOR "${HYPERION_VERSION_MAJOR}")
SET ( CPACK_PACKAGE_VERSION_MINOR "${HYPERION_VERSION_MINOR}")
SET ( CPACK_PACKAGE_VERSION_PATCH "${HYPERION_VERSION_PATCH}")
SET ( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE" )
SET ( CPACK_PACKAGE_EXECUTABLES "hyperiond;Hyperion" )
SET ( CPACK_CREATE_DESKTOP_LINKS "hyperiond;Hyperion" )

# Append pre-release version to CPACK_PACKAGE_VERSION_PATCH if exists
if (NOT "${HYPERION_VERSION_PRE}" STREQUAL "")
	string(APPEND CPACK_PACKAGE_VERSION_PATCH ${HYPERION_VERSION_PRE})
endif()

# Specific CPack Package Generators
# https://gitlab.kitware.com/cmake/community/-/wikis/doc/cpack/PackageGenerators

# .deb files for apt
# https://cmake.org/cmake/help/latest/cpack_gen/deb.html
SET ( CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_SOURCE_DIR}/cmake/package-scripts/preinst;${CMAKE_SOURCE_DIR}/cmake/package-scripts/postinst;${CMAKE_SOURCE_DIR}/cmake/package-scripts/prerm" )
SET ( CPACK_DEBIAN_PACKAGE_DEPENDS "libcec6 | libcec4 | libcec (>= 4.0)" )
SET ( CPACK_DEBIAN_PACKAGE_SECTION "Miscellaneous" )

# .rpm for rpm
# https://cmake.org/cmake/help/latest/cpack_gen/rpm.html
SET ( CPACK_RPM_PACKAGE_RELEASE 1 )
SET ( CPACK_RPM_PACKAGE_LICENSE "MIT" )
SET ( CPACK_RPM_PACKAGE_GROUP "Applications" )
SET ( CPACK_RPM_PACKAGE_REQUIRES "libcec >= 4.0.0" )
SET ( CPACK_RPM_PRE_INSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/cmake/package-scripts/preinst" )
SET ( CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/cmake/package-scripts/postinst" )
SET ( CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/cmake/package-scripts/prerm" )

# .dmg for Apple macOS
# https://cmake.org/cmake/help/latest/cpack_gen/dmg.html
IF (APPLE)
	SET ( CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/cmake/osxbundle/PackageIcon.icns" )
	SET ( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_BINARY_DIR}/LICENSE" )
	SET ( CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_SOURCE_DIR}/cmake/osxbundle/Background.png" )
	SET ( CPACK_DMG_DS_STORE_SETUP_SCRIPT "${CMAKE_SOURCE_DIR}/cmake/osxbundle/AppleScript.scpt" )
ENDIF(APPLE)

# Windows NSIS
# Use custom script based on cpack nsis template
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/nsis/template ${CMAKE_MODULE_PATH})

# Some path transformations
if(WIN32)
	file(TO_NATIVE_PATH ${CPACK_PACKAGE_ICON} CPACK_PACKAGE_ICON)
	STRING(REGEX REPLACE "\\\\" "\\\\\\\\" CPACK_PACKAGE_ICON ${CPACK_PACKAGE_ICON})
endif()
file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}/cmake/nsis/installer.ico" NSIS_HYP_ICO)
file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}/cmake/nsis/header.bmp" NSIS_HYP_LOGO_HORI)
file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}/cmake/nsis/logo.bmp" NSIS_HYP_LOGO_VERT)
STRING(REGEX REPLACE "\\\\" "\\\\\\\\" NSIS_HYP_ICO "${NSIS_HYP_ICO}")
STRING(REGEX REPLACE "\\\\" "\\\\\\\\" NSIS_HYP_LOGO_VERT "${NSIS_HYP_LOGO_VERT}")
STRING(REGEX REPLACE "\\\\" "\\\\\\\\" NSIS_HYP_LOGO_HORI "${NSIS_HYP_LOGO_HORI}")

SET ( CPACK_NSIS_MODIFY_PATH ON )
SET ( CPACK_NSIS_MUI_ICON ${NSIS_HYP_ICO})
SET ( CPACK_NSIS_MUI_UNIICON ${NSIS_HYP_ICO})
SET ( CPACK_NSIS_MUI_HEADERIMAGE ${NSIS_HYP_LOGO_HORI} )
SET ( CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP ${NSIS_HYP_LOGO_VERT})
SET ( CPACK_NSIS_DISPLAY_NAME "Hyperion Ambient Light")
SET ( CPACK_NSIS_PACKAGE_NAME "Hyperion" )
SET ( CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\hyperiond.exe")
SET ( CPACK_NSIS_HELP_LINK "https://www.hyperion-project.org")
SET ( CPACK_NSIS_URL_INFO_ABOUT "https://www.hyperion-project.org")
SET ( CPACK_NSIS_MUI_FINISHPAGE_RUN "hyperiond.exe")
SET ( CPACK_NSIS_BRANDING_TEXT "Hyperion-${HYPERION_VERSION}")

# custom nsis plugin directory
SET ( CPACK_NSIS_EXTRA_DEFS "!addplugindir ${CMAKE_SOURCE_DIR}/cmake/nsis/plugins")

# additional hyperiond startmenu link, won't be created if the user disables startmenu links
SET ( CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Hyperion (Console).lnk' '$INSTDIR\\\\bin\\\\hyperiond.exe' '-d -c'")
SET ( CPACK_NSIS_DELETE_ICONS_EXTRA "Delete '$SMPROGRAMS\\\\$MUI_TEMP\\\\Hyperion (Console).lnk'")

# Define the install components
# See also https://gitlab.kitware.com/cmake/community/-/wikis/doc/cpack/Component-Install-With-CPack
# and https://cmake.org/cmake/help/latest/module/CPackComponent.html
SET ( CPACK_COMPONENTS_GROUPING "ALL_COMPONENTS_IN_ONE")

# Components base (All builds)
SET ( CPACK_COMPONENTS_ALL "Hyperion" )

# Optional compiled

if(ENABLE_REMOTE_CTL)
	SET ( CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} "hyperion_remote" )
endif()

# only include standalone grabber with build was with flatbuffer client
if(ENABLE_FLATBUF_CONNECT)
	if(ENABLE_QT)
		SET ( CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} "hyperion_qt" )
	endif()
	if(ENABLE_AMLOGIC)
		SET ( CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} "hyperion_aml" )
	endif()
	if(ENABLE_V4L2)
		SET ( CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} "hyperion_v4l2" )
	endif()
	if(ENABLE_AUDIO)
		SET ( CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} "hyperion_audio" )
	endif()
	if(ENABLE_X11)
		SET ( CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} "hyperion_x11" )
	endif()
	if(ENABLE_XCB)
		SET ( CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} "hyperion_xcb" )
	endif()
	if(ENABLE_DISPMANX)
		SET ( CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} "hyperion_dispmanx" )
	endif()
	if(ENABLE_FB)
		SET ( CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} "hyperion_framebuffer" )
	endif()
	if(ENABLE_OSX)
		SET ( CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} "hyperion_osx" )
	endif()
endif(ENABLE_FLATBUF_CONNECT)

# Only include Hyperion to macOS dmg package (without standalone programs)
IF ( CPACK_GENERATOR MATCHES "DragNDrop" )
	LIST ( REMOVE_ITEM CPACK_COMPONENTS_ALL "hyperion_remote" "hyperion_qt" "hyperion_osx" )
ENDIF()

SET ( CPACK_ARCHIVE_COMPONENT_INSTALL ON )
SET ( CPACK_DEB_COMPONENT_INSTALL ON )
SET ( CPACK_RPM_COMPONENT_INSTALL ON )

SET ( CPACK_STRIP_FILES ON )

# no code after following line!
INCLUDE ( CPack )

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
