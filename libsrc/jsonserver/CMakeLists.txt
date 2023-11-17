add_library(jsonserver
	${CMAKE_SOURCE_DIR}/include/jsonserver/JsonServer.h
	${CMAKE_SOURCE_DIR}/libsrc/jsonserver/JsonServer.cpp
	${CMAKE_SOURCE_DIR}/libsrc/jsonserver/JsonClientConnection.h
	${CMAKE_SOURCE_DIR}/libsrc/jsonserver/JsonClientConnection.cpp
)

target_link_libraries(jsonserver
	hyperion-api
	hyperion
)

if(ENABLE_MDNS)
	target_link_libraries(jsonserver mdns)
endif()
