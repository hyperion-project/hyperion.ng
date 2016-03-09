
# Define the current source locations
set(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/protoserver)
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/protoserver)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${PROTOBUF_INCLUDE_DIRS}
)

# Group the headers that go through the MOC compiler
set(ProtoServer_QT_HEADERS
	${CURRENT_HEADER_DIR}/ProtoServer.h
	${CURRENT_HEADER_DIR}/ProtoConnection.h
	${CURRENT_SOURCE_DIR}/ProtoClientConnection.h
	${CURRENT_HEADER_DIR}/ProtoConnectionWrapper.h
)

set(ProtoServer_HEADERS
)

set(ProtoServer_SOURCES
	${CURRENT_SOURCE_DIR}/ProtoServer.cpp
	${CURRENT_SOURCE_DIR}/ProtoClientConnection.cpp
	${CURRENT_SOURCE_DIR}/ProtoConnection.cpp
	${CURRENT_SOURCE_DIR}/ProtoConnectionWrapper.cpp
)

set(ProtoServer_PROTOS
	${CURRENT_SOURCE_DIR}/message.proto
)

protobuf_generate_cpp(ProtoServer_PROTO_SRCS ProtoServer_PROTO_HDRS
	${ProtoServer_PROTOS}
)

if(ENABLE_QT5)
qt5_wrap_cpp(ProtoServer_HEADERS_MOC ${ProtoServer_QT_HEADERS})
else(ENABLE_QT5)
qt4_wrap_cpp(ProtoServer_HEADERS_MOC ${ProtoServer_QT_HEADERS})
endif(ENABLE_QT5)

add_library(protoserver
	${ProtoServer_HEADERS}
	${ProtoServer_QT_HEADERS}
	${ProtoServer_SOURCES}
	${ProtoServer_HEADERS_MOC}
	${ProtoServer_PROTOS}
	${ProtoServer_PROTO_SRCS}
	${ProtoServer_PROTO_HDRS}
)
if(ENABLE_QT5)
qt5_use_modules(protoserver Widgets)
endif(ENABLE_QT5)

target_link_libraries(protoserver
	hyperion
	hyperion-utils
	protobuf
	${QT_LIBRARIES}
)
