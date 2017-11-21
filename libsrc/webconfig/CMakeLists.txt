
# Define the current source locations
set(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/webconfig)
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/webconfig)

FILE ( GLOB WebConfig_SOURCES "${CURRENT_HEADER_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )
FILE ( GLOB_RECURSE webFiles RELATIVE ${CMAKE_BINARY_DIR}  ${CMAKE_SOURCE_DIR}/assets/webconfig/* )

FOREACH( f ${webFiles} )
    STRING ( REPLACE "../assets/webconfig/" "" fname ${f})
    SET(HYPERION_WEBCONFIG_RES "${HYPERION_WEBCONFIG_RES}\n\t\t<file alias=\"/webconfig/${fname}\">${f}</file>")
ENDFOREACH()
CONFIGURE_FILE(${CURRENT_SOURCE_DIR}/WebConfig.qrc.in ${CMAKE_BINARY_DIR}/WebConfig.qrc )
SET(WebConfig_RESOURCES ${CMAKE_BINARY_DIR}/WebConfig.qrc)

add_library(webconfig
	${WebConfig_SOURCES}
	${WebConfig_RESOURCES}
)

target_link_libraries(webconfig
	hyperion
	hyperion-utils
	Qt5::Network
)
