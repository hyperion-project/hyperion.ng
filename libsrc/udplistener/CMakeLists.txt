
# Define the current source locations
set(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/udplistener)
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/udplistener)

set(UDPListener_HEADERS
	${CURRENT_HEADER_DIR}/UDPListener.h
)

set(UDPListener_SOURCES
	${CURRENT_SOURCE_DIR}/UDPListener.cpp
)

add_library(udplistener
	${UDPListener_HEADERS}
	${UDPListener_SOURCES}
)

target_link_libraries(udplistener
	hyperion
	hyperion-utils
	${QT_LIBRARIES})
