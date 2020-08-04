cmake_minimum_required(VERSION 3.0.0)
project(hyperion-qt)

find_package(Qt5Widgets REQUIRED)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../../libsrc/flatbufserver
	${FLATBUFFERS_INCLUDE_DIRS}
)

set(Hyperion_QT_HEADERS
	QtWrapper.h
)

set(Hyperion_QT_SOURCES
	QtWrapper.cpp
	hyperion-qt.cpp
)

# generate windows .rc file for this binary
if (WIN32)
	include(${CMAKE_SOURCE_DIR}/cmake/win/win_rc.cmake)
	generate_win_rc_file(${PROJECT_NAME})
endif()

add_executable(${PROJECT_NAME}
	${Hyperion_QT_HEADERS}
	${Hyperion_QT_SOURCES}
	${${PROJECT_NAME}_WIN_RC_PATH}
)

target_link_libraries(${PROJECT_NAME}
	commandline
	qt-grabber
	flatbufserver
	flatbuffers
	ssdp
	Qt5::Core
	Qt5::Widgets
	Qt5::Network
)

if(NOT WIN32)
	install ( TARGETS ${PROJECT_NAME} DESTINATION "share/hyperion/bin" COMPONENT "hyperion_qt" )
else()
	install ( TARGETS ${PROJECT_NAME} DESTINATION "bin" COMPONENT "hyperion_qt" )
endif()

if(CMAKE_HOST_UNIX)
	install(CODE "EXECUTE_PROCESS(COMMAND ln -sf \"../share/hyperion/bin/${PROJECT_NAME}\" \"${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}\" )" COMPONENT  "hyperion_qt" )
	install(FILES "${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}" DESTINATION "bin" RENAME "${PROJECT_NAME}" COMPONENT "hyperion_qt" )
	install(CODE "FILE (REMOVE ${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME} )" COMPONENT  "hyperion_qt" )
endif(CMAKE_HOST_UNIX)
