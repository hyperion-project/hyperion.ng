
# Define the current source locations
set(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/boblightserver)
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/boblightserver)

# Group the headers that go through the MOC compiler
set(BoblightServer_QT_HEADERS
		${CURRENT_HEADER_DIR}/BoblightServer.h
		${CURRENT_SOURCE_DIR}/BoblightClientConnection.h
)

set(BoblightServer_HEADERS
)

set(BoblightServer_SOURCES
		${CURRENT_SOURCE_DIR}/BoblightServer.cpp
		${CURRENT_SOURCE_DIR}/BoblightClientConnection.cpp
)

qt4_wrap_cpp(BoblightServer_HEADERS_MOC ${BoblightServer_QT_HEADERS})

add_library(boblightserver
		${BoblightServer_HEADERS}
		${BoblightServer_QT_HEADERS}
		${BoblightServer_SOURCES}
		${BoblightServer_HEADERS_MOC}
)

target_link_libraries(boblightserver
		hyperion
		hyperion-utils)

qt4_use_modules(boblightserver
	Core
	Gui
	Network)
