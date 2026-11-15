find_package(PkgConfig REQUIRED)
pkg_check_modules(PIPEWIRE REQUIRED IMPORTED_TARGET libpipewire-0.3)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS DBus REQUIRED)

# ---------------------------------------------------------------------------
# Shared PipeWire frame-consumer (used by both wayland-grabber and gamescope-grabber)
# ---------------------------------------------------------------------------
add_library(pipewire-frame-source STATIC
	${CMAKE_SOURCE_DIR}/libsrc/grabber/pipewire/PipeWireFrameSource.cpp
	${CMAKE_SOURCE_DIR}/libsrc/grabber/pipewire/PipeWireFrameSource.h
)

target_link_libraries(pipewire-frame-source
	PUBLIC
		grabber
		Qt${QT_VERSION_MAJOR}::Core
		PkgConfig::PIPEWIRE
)

target_include_directories(pipewire-frame-source
	PUBLIC
		${CMAKE_SOURCE_DIR}/include
		${CMAKE_SOURCE_DIR}/libsrc/grabber/pipewire
)

# ---------------------------------------------------------------------------
# Wayland grabber (xdg-desktop-portal + PipeWire)
# ---------------------------------------------------------------------------
if(ENABLE_WAYLAND)
	add_library(wayland-grabber
		${CMAKE_SOURCE_DIR}/include/grabber/pipewire/WaylandGrabber.h
		${CMAKE_SOURCE_DIR}/include/grabber/pipewire/WaylandWrapper.h
		${CMAKE_SOURCE_DIR}/libsrc/grabber/pipewire/PortalScreenCast.cpp
		${CMAKE_SOURCE_DIR}/libsrc/grabber/pipewire/PortalScreenCast.h
		${CMAKE_SOURCE_DIR}/libsrc/grabber/pipewire/WaylandGrabber.cpp
		${CMAKE_SOURCE_DIR}/libsrc/grabber/pipewire/WaylandWrapper.cpp
	)

	target_link_libraries(wayland-grabber
		hyperion
		pipewire-frame-source
		Qt${QT_VERSION_MAJOR}::DBus
		PkgConfig::PIPEWIRE
	)
endif(ENABLE_WAYLAND)

# ---------------------------------------------------------------------------
# Gamescope grabber (direct PipeWire, no portal)
# ---------------------------------------------------------------------------
if(ENABLE_GAMESCOPE)
	add_library(gamescope-grabber
		${CMAKE_SOURCE_DIR}/include/grabber/pipewire/GamescopeGrabber.h
		${CMAKE_SOURCE_DIR}/include/grabber/pipewire/GamescopeWrapper.h
		${CMAKE_SOURCE_DIR}/libsrc/grabber/pipewire/GamescopeGrabber.cpp
		${CMAKE_SOURCE_DIR}/libsrc/grabber/pipewire/GamescopeWrapper.cpp
	)

	target_link_libraries(gamescope-grabber
		hyperion
		pipewire-frame-source
		PkgConfig::PIPEWIRE
	)
endif(ENABLE_GAMESCOPE)
