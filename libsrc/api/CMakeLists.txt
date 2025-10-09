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

target_link_libraries(hyperion-api
	hyperion
	hyperion-utils
)
