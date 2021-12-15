
# Define the current source locations
set(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/protoserver)
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/protoserver)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${PROTOBUF_INCLUDE_DIRS}
)

set(ProtoServer_PROTOS ${CURRENT_SOURCE_DIR}/message.proto )

protobuf_generate_cpp(ProtoServer_PROTO_SRCS ProtoServer_PROTO_HDRS ${ProtoServer_PROTOS} )

### Split protoconnect from protoserver as protoserver relates to HyperionDaemon and standalone capture binarys can't link to it

if( DEFAULT_PROTOBUF_CONNECT)
add_library(protoconnect
	${CURRENT_SOURCE_DIR}/ProtoClientConnection.h
	${CURRENT_SOURCE_DIR}/ProtoClientConnection.cpp
	${ProtoServer_PROTO_SRCS}
	${ProtoServer_PROTO_HDRS}
)
endif()

if(ENABLE_PROTOBUF_SERVER)
add_library(protoserver
	${CURRENT_HEADER_DIR}/ProtoServer.h
	${CURRENT_SOURCE_DIR}/ProtoServer.cpp
	${ProtoServer_PROTO_SRCS}
	${ProtoServer_PROTO_HDRS}
)
endif()

# disable warnings for auto generated proto files, we can't change the files ....
SET_SOURCE_FILES_PROPERTIES ( ${ProtoServer_PROTO_SRCS} ${ProtoServer_PROTO_HDRS} ${ProtoServer_PROTOS} PROPERTIES COMPILE_FLAGS -w )


if(ENABLE_PROTOBUF_CONNECT)
target_link_libraries(protoconnect
	hyperion
	hyperion-utils
	protobuf
	Qt${QT_VERSION_MAJOR}::Gui
)
endif()

if(ENABLE_PROTOBUF_SERVER)
target_link_libraries(protoserver
	hyperion
	hyperion-utils
	protobuf	
	Qt${QT_VERSION_MAJOR}::Gui
)
endif()
