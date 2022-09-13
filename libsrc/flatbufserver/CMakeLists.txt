
# Define the current source locations
set(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/flatbufserver)
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/flatbufserver)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}
	${FLATBUFFERS_INCLUDE_DIRS}
)

set(Flatbuffer_GENERATED_FBS
	hyperion_reply_generated.h
	hyperion_request_generated.h
)

set(Flatbuffer_FBS
	${CURRENT_SOURCE_DIR}/hyperion_reply.fbs
	${CURRENT_SOURCE_DIR}/hyperion_request.fbs
)
FOREACH(FBS_FILE ${Flatbuffer_FBS})
	compile_flattbuffer_schema(${FBS_FILE} ${CMAKE_CURRENT_SOURCE_DIR})
ENDFOREACH(FBS_FILE)

# let cmake know about new generated source files
set_source_files_properties(
	${Flatbuffer_GENERATED_FBS} PROPERTIES GENERATED TRUE
)

### Split flatbufconnect from flatbufserver as flatbufserver relates to HyperionDaemon

if(ENABLE_FLATBUF_CONNECT)
add_library(flatbufconnect
	${CURRENT_HEADER_DIR}/FlatBufferConnection.h
	${CURRENT_SOURCE_DIR}/FlatBufferConnection.cpp
	${FLATBUFSERVER_SOURCES}
	${Flatbuffer_GENERATED_FBS}

)
target_link_libraries(flatbufconnect
	hyperion-utils
	flatbuffers
	Qt${QT_VERSION_MAJOR}::Network
	Qt${QT_VERSION_MAJOR}::Core
)
endif()

if(ENABLE_FLATBUF_SERVER)
add_library(flatbufserver
	${CURRENT_HEADER_DIR}/FlatBufferServer.h
	${CURRENT_SOURCE_DIR}/FlatBufferServer.cpp
	${CURRENT_SOURCE_DIR}/FlatBufferClient.h
	${CURRENT_SOURCE_DIR}/FlatBufferClient.cpp
	${FLATBUFSERVER_SOURCES}
	${Flatbuffer_GENERATED_FBS}
)

target_link_libraries(flatbufserver
hyperion-utils
flatbuffers
Qt${QT_VERSION_MAJOR}::Network
Qt${QT_VERSION_MAJOR}::Core
)

if(ENABLE_MDNS)
	target_link_libraries(flatbufserver mdns)
endif()

endif()

