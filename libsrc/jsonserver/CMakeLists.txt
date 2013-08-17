
# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/jsonserver)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/jsonserver)

# Group the headers that go through the MOC compiler
SET(JsonServer_QT_HEADERS
		${CURRENT_HEADER_DIR}/JsonServer.h
		${CURRENT_SOURCE_DIR}/JsonClientConnection.h
)

SET(JsonServer_HEADERS
)

SET(JsonServer_SOURCES
		${CURRENT_SOURCE_DIR}/JsonServer.cpp
		${CURRENT_SOURCE_DIR}/JsonClientConnection.cpp
)

qt4_wrap_cpp(JsonServer_HEADERS_MOC ${JsonServer_QT_HEADERS})

add_library(jsonserver
		${JsonServer_HEADERS}
		${JsonServer_QT_HEADERS}
		${JsonServer_HEADERS_MOC}
		${JsonServer_SOURCES}
)

target_link_libraries(jsonserver
		hyperion
		jsoncpp)

qt4_use_modules(jsonserver
	Core
	Gui
	Network)
