find_package(PkgConfig REQUIRED)
pkg_check_modules(PIPEWIRE REQUIRED IMPORTED_TARGET libpipewire-0.3)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS DBus REQUIRED)

add_library(wayland-grabber
	${CMAKE_SOURCE_DIR}/include/grabber/wayland/WaylandGrabber.h
	${CMAKE_SOURCE_DIR}/include/grabber/wayland/WaylandWrapper.h
	${CMAKE_SOURCE_DIR}/libsrc/grabber/wayland/PipeWireFrameSource.cpp
	${CMAKE_SOURCE_DIR}/libsrc/grabber/wayland/PipeWireFrameSource.h
	${CMAKE_SOURCE_DIR}/libsrc/grabber/wayland/PortalScreenCast.cpp
	${CMAKE_SOURCE_DIR}/libsrc/grabber/wayland/PortalScreenCast.h
	${CMAKE_SOURCE_DIR}/libsrc/grabber/wayland/WaylandGrabber.cpp
	${CMAKE_SOURCE_DIR}/libsrc/grabber/wayland/WaylandWrapper.cpp
)

target_link_libraries(wayland-grabber
	hyperion
	Qt${QT_VERSION_MAJOR}::DBus
	PkgConfig::PIPEWIRE
)
