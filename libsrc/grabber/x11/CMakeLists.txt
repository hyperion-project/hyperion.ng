# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/grabber)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/grabber/x11)

# Find X11
find_package(X11 REQUIRED)

include_directories( ${X11_INCLUDES} )

if(APPLE)
	include_directories("/opt/X11/include")
endif(APPLE)

FILE ( GLOB X11_SOURCES "${CURRENT_HEADER_DIR}/X11*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

add_library(x11-grabber ${X11_SOURCES} )

target_link_libraries(x11-grabber
	hyperion
	${X11_LIBRARIES}
	${X11_Xrandr_LIB}
	${X11_Xrender_LIB}
	${QT_LIBRARIES}
)
