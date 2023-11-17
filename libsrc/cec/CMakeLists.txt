find_package(CEC REQUIRED)
if(CEC_FOUND)
	list(GET CEC_LIBRARIES 0 CEC_LIBRARIES)
	add_definitions(-DCEC_LIBRARY="${CEC_LIBRARIES}")
else()
	message(FATAL_ERROR "libCEC not found")
endif()

add_library(cechandler
	${CMAKE_SOURCE_DIR}/include/cec/CECHandler.h
	${CMAKE_SOURCE_DIR}/libsrc/cec/CECHandler.cpp
)

target_link_libraries(cechandler
	Qt${QT_VERSION_MAJOR}::Core
	${CMAKE_DL_LIBS}
)
