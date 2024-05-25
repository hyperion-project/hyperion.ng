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

target_link_libraries(hyperion-api
	hyperion
	hyperion-utils
	${DIRECTX9_LIBRARIES}
)

target_include_directories(hyperion-api PRIVATE
	${DIRECTX9_INCLUDE_DIRS}
)
