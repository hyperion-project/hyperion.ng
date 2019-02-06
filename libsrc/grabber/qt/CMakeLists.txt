# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/grabber)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/grabber/qt)

find_package(Qt5Widgets REQUIRED)

include_directories( ${X11_INCLUDES} )

FILE ( GLOB QT_GRAB_SOURCES "${CURRENT_HEADER_DIR}/Qt*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

add_library(qt-grabber ${QT_GRAB_SOURCES} )

target_link_libraries(qt-grabber
	hyperion
	Qt5::Widgets
)
