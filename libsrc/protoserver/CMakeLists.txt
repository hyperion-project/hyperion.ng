# set and compile proto schema
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/message.proto)

# split protoclient from protoserver as protoserver relates to HyperionDaemon and standalone capture binarys can't link to it
add_library(protoclient
	${CMAKE_CURRENT_SOURCE_DIR}/ProtoClientConnection.h
	${CMAKE_CURRENT_SOURCE_DIR}/ProtoClientConnection.cpp
	${PROTO_SRCS}
	${PROTO_HDRS}
)

target_link_libraries(protoclient
	hyperion
	hyperion-utils
	ProtoBuf
	Qt${QT_VERSION_MAJOR}::Gui
)

target_include_directories(protoclient PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

add_library(protoserver
	${CMAKE_SOURCE_DIR}/include/protoserver/ProtoServer.h
	${CMAKE_CURRENT_SOURCE_DIR}/ProtoServer.cpp
)

target_link_libraries(protoserver
	hyperion
	hyperion-utils
	protoclient
	Qt${QT_VERSION_MAJOR}::Gui
)

target_include_directories(protoserver PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

if(ENABLE_MDNS)
	target_link_libraries(protoserver mdns)
endif()
