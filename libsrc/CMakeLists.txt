
# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc)

add_subdirectory(hyperion)
add_subdirectory(commandline)
add_subdirectory(blackborder)
add_subdirectory(jsonserver)

if(ENABLE_BOBLIGHT_SERVER)
	add_subdirectory(boblightserver)
endif()

if(ENABLE_FLATBUF_SERVER OR ENABLE_FLATBUF_CONNECT)
add_subdirectory(flatbufserver)
endif()

if(ENABLE_PROTOBUF_SERVER)
add_subdirectory(protoserver)
endif()

if(ENABLE_FORWARDER)
	add_subdirectory(forwarder)
endif()

add_subdirectory(leddevice)
add_subdirectory(utils)
add_subdirectory(effectengine)
add_subdirectory(grabber)
add_subdirectory(webserver)
add_subdirectory(db)
add_subdirectory(api)
add_subdirectory(python)

add_subdirectory(ssdp)

# QMdnsEngine is currently not supported under MacOS because no Devs
if(NOT APPLE)
	add_subdirectory(mdns)
endif()

if(ENABLE_AVAHI)
	add_subdirectory(bonjour)
endif()

if(ENABLE_CEC)
	add_subdirectory(cec)
endif()

if(ENABLE_EXPERIMENTAL)
	add_subdirectory(experimental)
endif()

