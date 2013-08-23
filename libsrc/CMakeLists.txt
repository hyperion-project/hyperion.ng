
# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc)

add_subdirectory(bootsequence)
add_subdirectory(dispmanx-grabber)
add_subdirectory(hyperion)
add_subdirectory(jsonserver)
add_subdirectory(utils)
add_subdirectory(xbmcvideochecker)
