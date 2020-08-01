# Define the current source locations
find_package(CEC REQUIRED)

SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/cec)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/cec)

FILE (GLOB CEC_SOURCES "${CURRENT_HEADER_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp")
add_library(cechandler ${CEC_SOURCES})

include_directories(${CEC_INCLUDE_DIRS})

target_link_libraries(cechandler
	dl
	${CEC_LIBRARIES}
	Qt5::Core
)

