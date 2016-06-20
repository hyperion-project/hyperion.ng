
# Define the current source locations
set(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/udplistener)
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/udplistener)

# Group the headers that go through the MOC compiler
set(UDPListener_QT_HEADERS
	${CURRENT_HEADER_DIR}/UDPListener.h
)

set(UDPListener_HEADERS
)

set(UDPListener_SOURCES
	${CURRENT_SOURCE_DIR}/UDPListener.cpp
)

qt5_wrap_cpp(UDPListener_HEADERS_MOC ${UDPListener_QT_HEADERS})

add_library(udplistener
	${UDPListener_HEADERS}
	${UDPListener_QT_HEADERS}
	${UDPListener_SOURCES}
	${UDPListener_HEADERS_MOC}
)

qt5_use_modules(udplistener Widgets)

target_link_libraries(udplistener
	hyperion
	hyperion-utils
	${QT_LIBRARIES})
