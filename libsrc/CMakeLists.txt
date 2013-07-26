
# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc)

add_library(bob2hyperion SHARED
		bob2hyperion.cpp)

target_link_libraries(bob2hyperion
		hyperion
		hyperion-utils)

add_subdirectory(hyperion)
add_subdirectory(hyperionpng)
add_subdirectory(utils)
