find_package(Qt${QT_VERSION_MAJOR} COMPONENTS DBus REQUIRED)

add_library(wayland-grabber
	${CMAKE_SOURCE_DIR}/libsrc/grabber/wayland/PortalScreenCast.cpp
	${CMAKE_SOURCE_DIR}/libsrc/grabber/wayland/PortalScreenCast.h
)

target_link_libraries(wayland-grabber
	Qt${QT_VERSION_MAJOR}::DBus
)
