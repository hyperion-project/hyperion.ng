# cmake file for generating distribution packages

SET ( CPACK_GENERATOR "DEB" "TGZ" "STGZ" "Bundle") # "RPM"

SET ( CPACK_PACKAGE_NAME "hyperion" )
SET ( CPACK_PACKAGE_DESCRIPTION_SUMMARY "Hyperion is an open source ambient light implementation" )
SET ( CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md" )
SET ( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" )

SET ( CPACK_DEBIAN_PACKAGE_MAINTAINER "Hyperion Team")
SET ( CPACK_DEBIAN_PACKAGE_NAME "Hyperion" )
SET ( CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian/postinst;${CMAKE_CURRENT_SOURCE_DIR}/cmake/debian/preinst" )
SET ( CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://www.hyperion-project.org" )
SET ( CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5core5a (>= 5.2.0), libqt5network5 (>= 5.2.0), libqt5gui5 (>= 5.2.0), libqt5serialport5 (>= 5.2.0), libavahi-core7 (>= 0.6.31), libavahi-compat-libdnssd1 (>= 0.6.31), libusb-1.0-0, libpython2.7, libc6" )
SET ( CPACK_DEBIAN_PACKAGE_SECTION "Miscellaneous" )

SET ( CPACK_RPM_PACKAGE_NAME "Hyperion" )
SET ( CPACK_RPM_PACKAGE_URL "https://github.com/hyperion-project/hyperion" )
SET ( CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/rpm/postinst" )

SET ( CPACK_PACKAGE_FILE_NAME "Hyperion")
SET ( CPACK_PACKAGE_ICON ${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos/Hyperion.icns )
SET ( CPACK_BUNDLE_NAME "Hyperion" )
SET ( CPACK_BUNDLE_ICON ${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos/Hyperion.icns )
SET ( CPACK_BUNDLE_PLIST - path to a file that will become the bundle plist. )
SET ( CPACK_BUNDLE_STARTUP_COMMAND - path to a file that will be executed when the user opens the bundle. Could be a shell-script or a binary.

SET(CPACK_PACKAGE_VERSION_MAJOR "${HYPERION_VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${HYPERION_VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${HYPERION_VERSION_PATCH}")

SET ( CPACK_COMPONENTS_ALL "${PLATFORM}" )
SET ( CPACK_ARCHIVE_COMPONENT_INSTALL ON )
SET ( CPACK_DEB_COMPONENT_INSTALL ON )
SET ( CPACK_RPM_COMPONENT_INSTALL ON )
SET ( CPACK_STRIP_FILES ON )

# no code after following line!
INCLUDE ( CPack )
