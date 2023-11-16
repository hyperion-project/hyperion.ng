
file(GLOB_RECURSE webFiles RELATIVE ${CMAKE_BINARY_DIR}  ${CMAKE_SOURCE_DIR}/assets/webconfig/*)
file(RELATIVE_PATH webConfigPath ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}/assets/webconfig)

foreach(f ${webFiles})
	string(REPLACE "${webConfigPath}/" "" fname ${f})
	set(HYPERION_WEBCONFIG_RES "${HYPERION_WEBCONFIG_RES}\n\t\t<file alias=\"/webconfig/${fname}\">${f}</file>")
endforeach()

configure_file(${CMAKE_SOURCE_DIR}/libsrc/webserver/WebConfig.qrc.in ${CMAKE_BINARY_DIR}/WebConfig.qrc)

add_library(webserver
	${CMAKE_SOURCE_DIR}/include/webserver/WebServer.h
	${CMAKE_SOURCE_DIR}/libsrc/webserver/WebServer.cpp
	${CMAKE_SOURCE_DIR}/libsrc/webserver/CgiHandler.h
	${CMAKE_SOURCE_DIR}/libsrc/webserver/CgiHandler.cpp
	${CMAKE_SOURCE_DIR}/libsrc/webserver/QtHttpClientWrapper.h
	${CMAKE_SOURCE_DIR}/libsrc/webserver/QtHttpClientWrapper.cpp
	${CMAKE_SOURCE_DIR}/libsrc/webserver/QtHttpHeader.h
	${CMAKE_SOURCE_DIR}/libsrc/webserver/QtHttpHeader.cpp
	${CMAKE_SOURCE_DIR}/libsrc/webserver/QtHttpReply.h
	${CMAKE_SOURCE_DIR}/libsrc/webserver/QtHttpReply.cpp
	${CMAKE_SOURCE_DIR}/libsrc/webserver/QtHttpRequest.h
	${CMAKE_SOURCE_DIR}/libsrc/webserver/QtHttpRequest.cpp
	${CMAKE_SOURCE_DIR}/libsrc/webserver/QtHttpServer.h
	${CMAKE_SOURCE_DIR}/libsrc/webserver/QtHttpServer.cpp
	${CMAKE_SOURCE_DIR}/libsrc/webserver/StaticFileServing.h
	${CMAKE_SOURCE_DIR}/libsrc/webserver/StaticFileServing.cpp
	${CMAKE_SOURCE_DIR}/libsrc/webserver/WebJsonRpc.h
	${CMAKE_SOURCE_DIR}/libsrc/webserver/WebJsonRpc.cpp
	${CMAKE_SOURCE_DIR}/libsrc/webserver/WebSocketClient.h
	${CMAKE_SOURCE_DIR}/libsrc/webserver/WebSocketClient.cpp
	${CMAKE_SOURCE_DIR}/libsrc/webserver/WebSocketUtils.h
	${CMAKE_BINARY_DIR}/WebConfig.qrc
)

target_link_libraries(webserver
	hyperion
	hyperion-utils
	hyperion-api
)

if(ENABLE_MDNS)
	target_link_libraries(webserver mdns)
endif()
