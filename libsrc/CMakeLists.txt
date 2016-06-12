
# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc)

add_subdirectory(hyperion)
add_subdirectory(blackborder)
add_subdirectory(jsonserver)

add_subdirectory(protoserver)

if (ENABLE_ZEROCONF)
	add_subdirectory(bonjour)
endif (ENABLE_ZEROCONF)

add_subdirectory(boblightserver)
add_subdirectory(leddevice)
add_subdirectory(utils)
add_subdirectory(xbmcvideochecker)
add_subdirectory(effectengine)
add_subdirectory(grabber)

if(ENABLE_QT5)
	add_subdirectory(webconfig)
endif()
