
# Define the current source locations
set(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/mdns)
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/mdns)

FILE ( GLOB MDNS_SOURCES "${CURRENT_HEADER_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )
add_library(mdns ${MDNS_SOURCES})

include_directories(${QMDNS_INCLUDE_DIR})

target_link_libraries(mdns
hyperion-utils
${QMDNS_LIBRARIES}
)

target_include_directories(mdns PUBLIC ${QMDNS_INCLUDE_DIR})
