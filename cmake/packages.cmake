# cmake file for generating distribution packages

# default packages to build
IF (APPLE)
	SET ( CPACK_GENERATOR "TGZ" "Bundle")
ELSEIF (UNIX)
	SET ( CPACK_GENERATOR "TGZ")
ELSEIF (WIN32)
	SET ( CPACK_GENERATOR "ZIP")
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
SET ( CPACK_PACKAGE_EXECUTABLES "hyperiond;Hyperion" )
SET ( CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/resources/icons/hyperion-icon-32px.png")
SET ( CPACK_PACKAGE_VERSION_MAJOR "${HYPERION_VERSION_MAJOR}")
SET ( CPACK_PACKAGE_VERSION_MINOR "${HYPERION_VERSION_MINOR}")
SET ( CPACK_PACKAGE_VERSION_PATCH "${HYPERION_VERSION_PATCH}")
SET ( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" )
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

# NSIS for windows, requires NSIS TODO finish
SET ( CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/cmake/nsis/installer.ico")
SET ( CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/cmake/nsis/uninstaller.ico")
#SET ( CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/cmake/nsis/installer.bmp") #bmp required? If so, wrap in WIN32 check else: Use default icon instead
SET ( CPACK_NSIS_MODIFY_PATH ON)
SET ( CPACK_NSIS_DISPLAY_NAME "Hyperion Installer")
SET ( CPACK_NSIS_INSTALLED_ICON_NAME "Link to .exe")
SET ( CPACK_NSIS_HELP_LINK "https://www.hyperion-project.org")
SET ( CPACK_NSIS_URL_INFO_ABOUT "https://www.hyperion-project.org")

# define the install components
SET ( CPACK_COMPONENTS_ALL "${PLATFORM}" )

SET ( CPACK_COMPONENT_${PLATFORM}_ARCHIVE_FILE "${CPACK_PACKAGE_FILE_NAME}" )
SET ( CPACK_ARCHIVE_COMPONENT_INSTALL ON )

SET ( CPACK_DEBIAN_${PLATFORM}_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}.deb" )
SET ( CPACK_DEB_COMPONENT_INSTALL ON )

SET ( CPACK_RPM_${PLATFORM}_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}.rpm" )
SET ( CPACK_RPM_COMPONENT_INSTALL ON )

SET ( CPACK_STRIP_FILES ON )

# no code after following line!
INCLUDE ( CPack )
