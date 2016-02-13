
# Define the current source locations
set(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/jsonserver)
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/jsonserver)

# Group the headers that go through the MOC compiler
set(JsonServer_QT_HEADERS
		${CURRENT_HEADER_DIR}/JsonServer.h
		${CURRENT_SOURCE_DIR}/JsonClientConnection.h
)

set(JsonServer_HEADERS
)

set(JsonServer_SOURCES
		${CURRENT_SOURCE_DIR}/JsonServer.cpp
		${CURRENT_SOURCE_DIR}/JsonClientConnection.cpp
)

set(JsonServer_RESOURCES
		${CURRENT_SOURCE_DIR}/JsonSchemas.qrc
)
if(ENABLE_QT5)
qt5_wrap_cpp(JsonServer_HEADERS_MOC ${JsonServer_QT_HEADERS})
qt5_add_resources(JsonServer_RESOURCES_RCC ${JsonServer_RESOURCES} OPTIONS "-no-compress")
else(ENABLE_QT5)
qt4_wrap_cpp(JsonServer_HEADERS_MOC ${JsonServer_QT_HEADERS})
qt4_add_resources(JsonServer_RESOURCES_RCC ${JsonServer_RESOURCES} OPTIONS "-no-compress")
endif(ENABLE_QT5)

add_library(jsonserver
		${JsonServer_HEADERS}
		${JsonServer_QT_HEADERS}
		${JsonServer_SOURCES}
		${JsonServer_RESOURCES}
		${JsonServer_HEADERS_MOC}
		${JsonServer_RESOURCES_RCC}
)

if(ENABLE_QT5)
qt5_use_modules(jsonserver Widgets Network)
endif(ENABLE_QT5)

target_link_libraries(jsonserver
		hyperion
		hyperion-utils
        jsoncpp
        ${QT_LIBRARIES})
