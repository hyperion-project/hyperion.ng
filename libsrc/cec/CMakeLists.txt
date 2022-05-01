# Define the current source locations
find_package(CEC REQUIRED)

SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/cec)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/cec)

FILE (GLOB CEC_SOURCES "${CURRENT_HEADER_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp")

add_library(cechandler ${CEC_SOURCES})
add_definitions(-DCEC_LIBRARY="${CEC_LIBRARIES}")

include_directories(${CEC_INCLUDE_DIRS})

target_link_libraries(cechandler
	Qt${QT_VERSION_MAJOR}::Core
	${CMAKE_DL_LIBS}
)

