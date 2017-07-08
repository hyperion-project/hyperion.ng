
# Define the current source locations
set(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/jsonserver)
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/jsonserver)

set(JsonServer_HEADERS
	${CURRENT_HEADER_DIR}/JsonServer.h
	${CURRENT_SOURCE_DIR}/JsonClientConnection.h
)

set(JsonServer_SOURCES
	${CURRENT_SOURCE_DIR}/JsonServer.cpp
	${CURRENT_SOURCE_DIR}/JsonClientConnection.cpp
)

add_library(jsonserver
	${JsonServer_HEADERS}
	${JsonServer_SOURCES}
)

target_link_libraries(jsonserver
	hyperion
	Qt5::Network
	Qt5::Gui)
