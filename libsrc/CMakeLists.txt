
# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc)

add_subdirectory(hyperion)
add_subdirectory(blackborder)
add_subdirectory(jsonserver)

if (ENABLE_PROTOBUF)
	add_subdirectory(protoserver)
endif (ENABLE_PROTOBUF)

add_subdirectory(boblightserver)
add_subdirectory(leddevice)
add_subdirectory(utils)
add_subdirectory(xbmcvideochecker)
add_subdirectory(effectengine)
add_subdirectory(grabber)
