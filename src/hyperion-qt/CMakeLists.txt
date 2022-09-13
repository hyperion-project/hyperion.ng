cmake_minimum_required(VERSION 3.1.0)
project(hyperion-qt)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Network Widgets REQUIRED)

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
	flatbufconnect
	flatbuffers
	Qt${QT_VERSION_MAJOR}::Core
	Qt${QT_VERSION_MAJOR}::Gui
	Qt${QT_VERSION_MAJOR}::Network
	Qt${QT_VERSION_MAJOR}::Widgets
)

if(ENABLE_MDNS)
        target_link_libraries(${PROJECT_NAME} mdns)
else()
        target_link_libraries(${PROJECT_NAME} ssdp)
endif()

if(APPLE)
	install ( TARGETS ${PROJECT_NAME} DESTINATION "." COMPONENT "hyperion_qt" )
elseif(NOT WIN32)
	install ( TARGETS ${PROJECT_NAME} DESTINATION "share/hyperion/bin" COMPONENT "hyperion_qt" )
else()
	install ( TARGETS ${PROJECT_NAME} DESTINATION "bin" COMPONENT "hyperion_qt" )
endif()

if(CMAKE_HOST_UNIX  AND NOT APPLE)
	install(CODE "EXECUTE_PROCESS(COMMAND ln -sf \"../share/hyperion/bin/${PROJECT_NAME}\" \"${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}\" )" COMPONENT  "hyperion_qt" )
	install(FILES "${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}" DESTINATION "bin" RENAME "${PROJECT_NAME}" COMPONENT "hyperion_qt" )
	install(CODE "FILE (REMOVE ${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME} )" COMPONENT  "hyperion_qt" )
endif()
