add_library(hyperion-api
	${CMAKE_SOURCE_DIR}/include/api/apiStructs.h
	${CMAKE_SOURCE_DIR}/include/api/API.h
	${CMAKE_SOURCE_DIR}/include/api/JsonAPI.h
	${CMAKE_SOURCE_DIR}/include/api/JsonCallbacks.h
	${CMAKE_SOURCE_DIR}/include/api/JsonApiCommand.h
	${CMAKE_SOURCE_DIR}/include/api/JsonApiSubscription.h
	${CMAKE_SOURCE_DIR}/include/api/JsonInfo.h
	${CMAKE_SOURCE_DIR}/libsrc/api/JsonAPI.cpp
	${CMAKE_SOURCE_DIR}/libsrc/api/API.cpp
	${CMAKE_SOURCE_DIR}/libsrc/api/JsonCallbacks.cpp
	${CMAKE_SOURCE_DIR}/libsrc/api/JsonInfo.cpp
	${CMAKE_SOURCE_DIR}/libsrc/api/JSONRPC_schemas.qrc
)

if(ENABLE_DX)
	include_directories(${DIRECTX9_INCLUDE_DIRS})
	target_link_libraries(hyperion-api ${DIRECTX9_LIBRARIES})
endif(ENABLE_DX)

if(ENABLE_DRM)
	include_directories(${LIBDRM_INCLUDE_DIRS})
	target_link_libraries(hyperion-api drm-grabber)
endif(ENABLE_DRM)

if(ENABLE_GAMESCOPE)
	# GrabberConfig.h (included by JsonInfo.cpp for discoverGrabber<GamescopeGrabber>())
	# pulls in <pipewire/pipewire.h> - only the include path is needed here since
	# hyperion-api is a static library; actual symbol resolution happens when
	# hyperiond links gamescope-grabber (see src/hyperiond/CMakeLists.txt).
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)
	include_directories(${PIPEWIRE_INCLUDE_DIRS})
endif(ENABLE_GAMESCOPE)

if(ENABLE_DESKTOP_PORTAL)
	# Same rationale as the ENABLE_GAMESCOPE block above: GrabberConfig.h transitively pulls in
	# <pipewire/pipewire.h> via DesktopPortalGrabber.h for discoverGrabber<DesktopPortalGrabber>().
	# DesktopPortalGrabber.h itself has no QtDBus types, so no D-Bus include path is needed here -
	# that's confined to DesktopPortalGrabber.cpp, built only by desktopportal-grabber.
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)
	include_directories(${PIPEWIRE_INCLUDE_DIRS})
endif(ENABLE_DESKTOP_PORTAL)

target_link_libraries(hyperion-api
	hyperion
	hyperion-utils
)
