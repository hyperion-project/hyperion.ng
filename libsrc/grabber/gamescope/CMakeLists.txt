find_package(PkgConfig REQUIRED)
pkg_check_modules(PIPEWIRE REQUIRED IMPORTED_TARGET libpipewire-0.3)

add_library(gamescope-grabber
	${CMAKE_SOURCE_DIR}/include/grabber/gamescope/GamescopeGrabber.h
	${CMAKE_SOURCE_DIR}/include/grabber/gamescope/GamescopeWrapper.h
	${CMAKE_SOURCE_DIR}/libsrc/grabber/gamescope/GamescopeGrabber.cpp
	${CMAKE_SOURCE_DIR}/libsrc/grabber/gamescope/GamescopeWrapper.cpp
)

target_link_libraries(gamescope-grabber
	hyperion
	PkgConfig::PIPEWIRE
)
