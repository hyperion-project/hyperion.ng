# Define the current source locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/grabber)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/grabber/mediafoundation)

FILE ( GLOB MF_SOURCES "${CURRENT_HEADER_DIR}/MF*.h" "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

add_library(mf-grabber ${MF_SOURCES} )

target_link_libraries(mf-grabber
	hyperion
	${QT_LIBRARIES}
)

if(TURBOJPEG_FOUND)
	target_link_libraries(mf-grabber ${TurboJPEG_LIBRARY})
endif(TURBOJPEG_FOUND)
