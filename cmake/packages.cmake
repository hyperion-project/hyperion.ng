# cmake file for generating distribution packages

IF (APPLE)
	SET ( CPACK_GENERATOR "TGZ" "Bundle")
ELSEIF (UNIX)
	SET ( CPACK_GENERATOR "DEB" "TGZ" "STGZ") # "RPM"
ELSEIF (WIN32)
	SET ( CPACK_GENERATOR "ZIP" "NSIS")
ENDIF()

# Apply to all packages, some of these can be overwritten with generator specific content
# https://cmake.org/cmake/help/v3.0/module/CPack.html

SET ( CPACK_PACKAGE_NAME "Hyperion" )
SET ( CPACK_PACKAGE_DESCRIPTION_SUMMARY "Hyperion is an open source ambient light implementation" )
SET ( CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md" )
SET ( CPACK_PACKAGE_FILE_NAME "Hyperion-${HYPERION_VERSION_MAJOR}.${HYPERION_VERSION_MINOR}.${HYPERION_VERSION_PATCH}")
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
# .deb files for dpkg

SET ( CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian/preinst;${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian/postinst;${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian/prerm" )
SET ( CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5core5a (>= 5.5.0), libqt5network5 (>= 5.5.0), libqt5gui5 (>= 5.5.0), libqt5serialport5 (>= 5.5.0), libqt5sql5 (>= 5.5.0), libqt5sql5-sqlite (>= 5.5.0), libavahi-core7 (>= 0.6.31), libavahi-compat-libdnssd1 (>= 0.6.31), libusb-1.0-0, libpython3.5, libc6" )
SET ( CPACK_DEBIAN_PACKAGE_SECTION "Miscellaneous" )

# .rpm for rpm
SET ( CPACK_RPM_PACKAGE_RELEASE 1)
SET ( CPACK_RPM_PACKAGE_LICENSE "unknown")
SET ( CPACK_RPM_PACKAGE_GROUP "unknown")
SET ( CPACK_RPM_PACKAGE_REQUIRES "libqt5core5a >= 5.5.0, libqt5network5 >= 5.5.0, libqt5gui5 >= 5.5.0, libqt5serialport5 >= 5.5.0, libqt5sql5 >= 5.5.0, libqt5sql5-sqlite >= 5.5.0, libavahi-core7 >= 0.6.31, libavahi-compat-libdnssd1 >= 0.6.31, libusb-1.0-0, libpython3.5, libc6")
SET ( CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/rpm/postinst" )

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
SET ( CPACK_ARCHIVE_COMPONENT_INSTALL ON )
SET ( CPACK_DEB_COMPONENT_INSTALL ON )
SET ( CPACK_RPM_COMPONENT_INSTALL ON )
SET ( CPACK_STRIP_FILES ON )

# no code after following line!
INCLUDE ( CPack )
