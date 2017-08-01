
# Define the current source locations
set(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/webconfig)
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/webconfig)

set(WebConfig_HEADERS
	${CURRENT_SOURCE_DIR}/QtHttpClientWrapper.h
	${CURRENT_SOURCE_DIR}/QtHttpReply.h
	${CURRENT_SOURCE_DIR}/QtHttpRequest.h
	${CURRENT_SOURCE_DIR}/QtHttpServer.h
	${CURRENT_SOURCE_DIR}/CgiHandler.h
	${CURRENT_SOURCE_DIR}/StaticFileServing.h
	${CURRENT_HEADER_DIR}/WebConfig.h
)

set(WebConfig_SOURCES
	${CURRENT_SOURCE_DIR}/QtHttpClientWrapper.cpp
	${CURRENT_SOURCE_DIR}/QtHttpHeader.cpp
	${CURRENT_SOURCE_DIR}/QtHttpReply.cpp
	${CURRENT_SOURCE_DIR}/QtHttpRequest.cpp
	${CURRENT_SOURCE_DIR}/QtHttpServer.cpp
	${CURRENT_SOURCE_DIR}/CgiHandler.cpp
	${CURRENT_SOURCE_DIR}/StaticFileServing.cpp
	${CURRENT_SOURCE_DIR}/WebConfig.cpp
)
FILE ( GLOB_RECURSE webFiles RELATIVE ${CMAKE_BINARY_DIR}  ${CMAKE_SOURCE_DIR}/assets/webconfig/* )
FOREACH( f ${webFiles} )
    STRING ( REPLACE "../assets/webconfig/" "" fname ${f})
    SET(HYPERION_WEBCONFIG_RES "${HYPERION_WEBCONFIG_RES}\n\t\t<file alias=\"/webconfig/${fname}\">${f}</file>")
ENDFOREACH()
CONFIGURE_FILE(${CURRENT_SOURCE_DIR}/WebConfig.qrc.in ${CMAKE_BINARY_DIR}/WebConfig.qrc )
SET(WebConfig_RESOURCES ${CMAKE_BINARY_DIR}/WebConfig.qrc)

qt5_add_resources(WebConfig_RESOURCES_RCC ${WebConfig_RESOURCES} ) #OPTIONS "-no-compress"

add_library(webconfig
	${WebConfig_HEADERS}
	${WebConfig_SOURCES}
	${WebConfig_RESOURCES_RCC}
)

target_link_libraries(webconfig
	hyperion
	hyperion-utils
	Qt5::Network
)
