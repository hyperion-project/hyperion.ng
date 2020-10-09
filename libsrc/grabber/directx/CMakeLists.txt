# Define the current source locations
SET( CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/grabber )
SET( CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/grabber/directx )

include_directories(${DIRECTX9_INCLUDE_DIRS})

FILE ( GLOB DIRECTX_GRAB_SOURCES "${CURRENT_HEADER_DIR}/DirectX*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

add_library( directx-grabber ${DIRECTX_GRAB_SOURCES} )

target_link_libraries(directx-grabber
    hyperion
    ${DIRECTX9_LIBRARIES}
)
