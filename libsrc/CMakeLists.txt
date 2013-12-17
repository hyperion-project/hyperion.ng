
# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc)

add_subdirectory(hyperion)
add_subdirectory(jsonserver)
add_subdirectory(protoserver)
add_subdirectory(boblightserver)
add_subdirectory(leddevice)
add_subdirectory(utils)
add_subdirectory(xbmcvideochecker)
add_subdirectory(effectengine)

if (ENABLE_DISPMANX)
	add_subdirectory(dispmanx-grabber)
endif (ENABLE_DISPMANX)
