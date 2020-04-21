# cmake file for generating distribution packages

# default packages to build
IF (APPLE)
	SET ( CPACK_GENERATOR "TGZ" "Bundle")
ELSEIF (UNIX)
	SET ( CPACK_GENERATOR "TGZ" "STGZ")
ELSEIF (WIN32)
	SET ( CPACK_GENERATOR "ZIP" "NSIS")
ENDIF()

# Determine packages by found generator executables
find_package(RpmBuilder)
find_package(DebBuilder)
IF(RPM_BUILDER_FOUND)
	message(STATUS "CPACK: Found RPM builder")
	SET ( CPACK_GENERATOR ${CPACK_GENERATOR} "RPM")
ENDIF()
IF(DEB_BUILDER_FOUND)
	message(STATUS "CPACK: Found DEB builder")
	SET ( CPACK_GENERATOR ${CPACK_GENERATOR} "DEB")
ENDIF()

# Apply to all packages, some of these can be overwritten with generator specific content
# https://cmake.org/cmake/help/v3.5/module/CPack.html

SET ( CPACK_PACKAGE_NAME "Hyperion" )
SET ( CPACK_PACKAGE_DESCRIPTION_SUMMARY "Hyperion is an open source ambient light implementation" )
SET ( CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md" )

IF ( NOT DEFINED DOCKER_PLATFORM )
	SET ( CPACK_PACKAGE_FILE_NAME "Hyperion-${HYPERION_VERSION}-${CMAKE_SYSTEM_NAME}")
ELSE()
	SET ( CPACK_PACKAGE_FILE_NAME "Hyperion-${HYPERION_VERSION}-${CMAKE_SYSTEM_NAME}-${DOCKER_PLATFORM}")
ENDIF()

SET ( CPACK_PACKAGE_CONTACT "packages@hyperion-project.org")
SET ( CPACK_PACKAGE_VENDOR "hyperion-project")
SET ( CPACK_PACKAGE_EXECUTABLES "hyperiond;Hyperion" )
SET ( CPACK_PACKAGE_INSTALL_DIRECTORY "Hyperion" )
SET ( CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/resources/icons/hyperion-icon-32px.png")

SET ( CPACK_PACKAGE_VERSION_MAJOR "${HYPERION_VERSION_MAJOR}")
SET ( CPACK_PACKAGE_VERSION_MINOR "${HYPERION_VERSION_MINOR}")
SET ( CPACK_PACKAGE_VERSION_PATCH "${HYPERION_VERSION_PATCH}")
SET ( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" )
SET ( CPACK_PACKAGE_EXECUTABLES "hyperiond;Hyperion" )
SET ( CPACK_CREATE_DESKTOP_LINKS "hyperiond;Hyperion" )

# Specific CPack Package Generators
# https://cmake.org/Wiki/CMake:CPackPackageGenerators
# .deb files for apt

SET ( CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian/preinst;${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian/postinst;${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian/prerm" )
SET ( CPACK_DEBIAN_PACKAGE_SECTION "Miscellaneous" )

# .rpm for rpm
# https://cmake.org/cmake/help/v3.5/module/CPackRPM.html
SET ( CPACK_RPM_PACKAGE_RELEASE 1)
SET ( CPACK_RPM_PACKAGE_LICENSE "MIT")
SET ( CPACK_RPM_PACKAGE_GROUP "Applications")
# Notes: This is a dependency list for Fedora 27, different .rpm OSes use different names for their deps
SET ( CPACK_RPM_PRE_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/rpm/preinst" )
SET ( CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/rpm/postinst" )
SET ( CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/rpm/prerm" )

# OSX "Bundle" generator TODO Add more osx generators
# https://cmake.org/cmake/help/v3.10/module/CPackBundle.html
SET ( CPACK_BUNDLE_NAME "Hyperion" )
SET ( CPACK_BUNDLE_ICON ${CMAKE_CURRENT_SOURCE_DIR}/cmake/osxbundle/Hyperion.icns )
SET ( CPACK_BUNDLE_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/cmake/osxbundle/Info.plist )
SET ( CPACK_BUNDLE_STARTUP_COMMAND "${CMAKE_SOURCE_DIR}/cmake/osxbundle/launch.sh" )

# Windows NSIS
# Some path transformations
if(WIN32)
	file(TO_NATIVE_PATH ${CPACK_PACKAGE_ICON} CPACK_PACKAGE_ICON)
	STRING(REGEX REPLACE "\\\\" "\\\\\\\\" CPACK_PACKAGE_ICON ${CPACK_PACKAGE_ICON})
endif()
file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/nsis/installer.ico" HYP_NSIS_ICO)
file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/nsis/hyperion-logo-white.bmp" HYP_NSIS_BMP)
STRING(REGEX REPLACE "\\\\" "\\\\\\\\" HYP_NSIS_ICO "${HYP_NSIS_ICO}")
STRING(REGEX REPLACE "\\\\" "\\\\\\\\" HYP_NSIS_BMP "${HYP_NSIS_BMP}")

SET ( CPACK_NSIS_MODIFY_PATH ON )
SET ( CPACK_NSIS_MUI_ICON ${HYP_NSIS_ICO})
SET ( CPACK_NSIS_MUI_UNIICON ${HYP_NSIS_ICO})
SET ( CPACK_NSIS_MUI_HEADERIMAGE ${HYP_NSIS_BMP} )
SET ( CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP ${HYP_NSIS_BMP})
SET ( CPACK_NSIS_DISPLAY_NAME "Hyperion Ambient Light")
SET ( CPACK_NSIS_PACKAGE_NAME "Hyperion" )
SET ( CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\hyperiond.exe")
SET ( CPACK_NSIS_HELP_LINK "https://www.hyperion-project.org")
SET ( CPACK_NSIS_URL_INFO_ABOUT "https://www.hyperion-project.org")
# hyperiond startmenu link
#SET ( CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Hyperion.lnk' '$INSTDIR\\\\bin\\\\hyperiond.exe'")
#SET ( CPACK_NSIS_DELETE_ICONS_EXTRA "Delete '$SMPROGRAMS\\\\$START_MENU\\\\Hyperion.lnk'")
# hyperiond desktop link
#SET ( CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '$DESKTOP\\\\Hyperion.lnk' '$INSTDIR\\\\bin\\\\hyperiond.exe' ")
#SET ( CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "Delete '$DESKTOP\\\\Hyperion.lnk' ")

# With cli args: SET ( CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Hyperion (Debug).lnk' '$INSTDIR\\\\bin\\\\hyperiond.exe' '-d'")
#SET ( CPACK_NSIS_EXTRA_INSTALL_COMMANDS "CreateShortCut \\\"$DESKTOP\\\\Hyperion.lnk\\\" \\\"$INSTDIR\\\\bin\\\\hyperiond.exe\\\" ")
#SET ( CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "Delete \\\"$DESKTOP\\\\Hyperion.lnk\\\" ")

# define the install components
# See also https://gitlab.kitware.com/cmake/community/-/wikis/doc/cpack/Component-Install-With-CPack
# and https://cmake.org/cmake/help/latest/module/CPackComponent.html
if(NOT WIN32)
	SET ( CPACK_COMPONENTS_ALL "${PLATFORM}" )
else()
	SET ( CPACK_COMPONENTS_GROUPING "ALL_COMPONENTS_IN_ONE")
	# Components base
	SET ( CPACK_COMPONENTS_ALL "Hyperion" "hyperion_remote" )
	# optional compiled
	if(ENABLE_QT)
		SET ( CPACK_COMPONENTS_ALL ${CPACK_COMPONENTS_ALL} "hyperion_qt" )
	endif()
endif()

SET ( CPACK_COMPONENT_${PLATFORM}_ARCHIVE_FILE "${CPACK_PACKAGE_FILE_NAME}" )
SET ( CPACK_ARCHIVE_COMPONENT_INSTALL ON )

SET ( CPACK_DEBIAN_${PLATFORM}_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}.deb" )
SET ( CPACK_DEB_COMPONENT_INSTALL ON )

SET ( CPACK_RPM_${PLATFORM}_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}.rpm" )
SET ( CPACK_RPM_COMPONENT_INSTALL ON )

SET ( CPACK_STRIP_FILES ON )

# no code after following line!
INCLUDE ( CPack )

if(WIN32)
	cpack_add_install_type(Full DISPLAY_NAME "Full")
	cpack_add_install_type(Min DISPLAY_NAME "Minimal")
	cpack_add_component_group(Runtime EXPANDED DESCRIPTION "Hyperion runtime and hyperion-remote commandline tool")
	cpack_add_component_group(Screencapture EXPANDED DESCRIPTION "Standalone Screencapture commandline programs")
	# Components base
	cpack_add_component(Hyperion
		DISPLAY_NAME "Hyperion"
		DESCRIPTION "Hyperion runtime"
		INSTALL_TYPES Full Min
		GROUP Runtime
		REQUIRED
	)
	cpack_add_component(hyperion_remote
		DISPLAY_NAME "Hyperion Remote"
		DESCRIPTION "Hyperion remote cli tool"
		INSTALL_TYPES Full
		GROUP Runtime
		DEPENDS Hyperion
	)

	# optional compiled
	if(ENABLE_QT)
		cpack_add_component(hyperion_qt
			DISPLAY_NAME "Qt Standalone Screencap"
			DESCRIPTION "Qt based standalone screen capture"
			INSTALL_TYPES Full
			GROUP Screencapture
			DEPENDS Hyperion
		)
	endif()
endif()
