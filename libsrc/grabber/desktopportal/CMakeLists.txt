find_package(PkgConfig REQUIRED)
pkg_check_modules(PIPEWIRE REQUIRED IMPORTED_TARGET libpipewire-0.3)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS DBus REQUIRED)

add_library(desktopportal-grabber
	${CMAKE_SOURCE_DIR}/include/grabber/desktopportal/DesktopPortalGrabber.h
	${CMAKE_SOURCE_DIR}/include/grabber/desktopportal/DesktopPortalWrapper.h
	${CMAKE_SOURCE_DIR}/libsrc/grabber/desktopportal/DesktopPortalGrabber.cpp
	${CMAKE_SOURCE_DIR}/libsrc/grabber/desktopportal/DesktopPortalWrapper.cpp
)

target_link_libraries(desktopportal-grabber
	hyperion
	PkgConfig::PIPEWIRE
	Qt${QT_VERSION_MAJOR}::DBus
)
