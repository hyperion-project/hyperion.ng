# Compile flatbuffer schemas
compile_flatbuffer_schema(Compiled_FBS
	${CMAKE_CURRENT_SOURCE_DIR}/hyperion_request.fbs
	${CMAKE_CURRENT_SOURCE_DIR}/hyperion_reply.fbs
)

# split flatbufconnect from flatbufserver as flatbufserver relates to HyperionDaemon
if(ENABLE_FLATBUF_CONNECT)
	add_library(flatbufconnect
		${CMAKE_SOURCE_DIR}/include/flatbufserver/FlatBufferConnection.h
		${CMAKE_SOURCE_DIR}/libsrc/flatbufserver/FlatBufferConnection.cpp
		${Compiled_FBS}
	)

	target_link_libraries(flatbufconnect
		hyperion-utils
		FlatBuffers
	)

	target_include_directories(flatbufconnect PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
endif()

if(ENABLE_FLATBUF_SERVER)
	add_library(flatbufserver
		${CMAKE_SOURCE_DIR}/include/flatbufserver/FlatBufferServer.h
		${CMAKE_SOURCE_DIR}/libsrc/flatbufserver/FlatBufferServer.cpp
		${CMAKE_SOURCE_DIR}/libsrc/flatbufserver/FlatBufferClient.h
		${CMAKE_SOURCE_DIR}/libsrc/flatbufserver/FlatBufferClient.cpp
		${Compiled_FBS}
	)

	target_link_libraries(flatbufserver
		hyperion-utils
		FlatBuffers
	)

	target_include_directories(flatbufserver PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

	if(ENABLE_MDNS)
		target_link_libraries(flatbufserver mdns)
	endif()
endif()
