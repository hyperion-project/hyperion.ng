# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/grabber)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/grabber/xcb)

find_package(XCB COMPONENTS SHM IMAGE RENDER RANDR REQUIRED)

include_directories(${XCB_INCLUDE_DIRS})

FILE (GLOB XCB_SOURCES "${CURRENT_HEADER_DIR}/Xcb*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

add_library(xcb-grabber ${XCB_SOURCES})

target_link_libraries(xcb-grabber
	hyperion
	${XCB_LIBRARIES}
	${QT_LIBRARIES}
)

if (NOT APPLE)
	target_link_libraries(
		xcb-grabber
	)
endif()
