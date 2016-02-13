
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

if(ENABLE_QT5)
qt5_wrap_cpp(BoblightServer_HEADERS_MOC ${BoblightServer_QT_HEADERS})
else(ENABLE_QT5)
qt4_wrap_cpp(BoblightServer_HEADERS_MOC ${BoblightServer_QT_HEADERS})
endif(ENABLE_QT5)

add_library(boblightserver
		${BoblightServer_HEADERS}
		${BoblightServer_QT_HEADERS}
		${BoblightServer_SOURCES}
		${BoblightServer_HEADERS_MOC}
)

if(ENABLE_QT5)
qt5_use_modules(boblightserver Widgets)
endif(ENABLE_QT5)

target_link_libraries(boblightserver
		hyperion
        hyperion-utils
        ${QT_LIBRARIES})
