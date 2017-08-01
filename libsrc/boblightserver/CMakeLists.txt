
# Define the current source locations
set(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/boblightserver)
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/boblightserver)

set(BoblightServer_HEADERS
	${CURRENT_HEADER_DIR}/BoblightServer.h
	${CURRENT_SOURCE_DIR}/BoblightClientConnection.h
)

set(BoblightServer_SOURCES
	${CURRENT_SOURCE_DIR}/BoblightServer.cpp
	${CURRENT_SOURCE_DIR}/BoblightClientConnection.cpp
)

add_library(boblightserver
	${BoblightServer_HEADERS}
	${BoblightServer_SOURCES}
)

target_link_libraries(boblightserver
	hyperion
	hyperion-utils
	${QT_LIBRARIES})
