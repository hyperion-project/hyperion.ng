# Define the current source locations
set(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/commandline)
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/commandline)

FILE ( GLOB Parser_SOURCES "${CURRENT_HEADER_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

add_library(commandline ${Parser_SOURCES} )

target_link_libraries(commandline
	hyperion
	Qt${QT_VERSION_MAJOR}::Gui
)
