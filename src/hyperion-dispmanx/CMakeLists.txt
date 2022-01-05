cmake_minimum_required(VERSION 3.0.0)
project(hyperion-dispmanx)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Network Widgets REQUIRED)

IF ( "${PLATFORM}" MATCHES rpi)
	find_package(BCM REQUIRED)
ELSE()
	SET(BCM_INCLUDE_DIRS "")
	SET(BCM_LIBRARIES "")
ENDIF()

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../../libsrc/flatbufserver
	${BCM_INCLUDE_DIRS}
	${FLATBUFFERS_INCLUDE_DIRS}
)

set(Hyperion_Dispmanx_HEADERS
	DispmanxWrapper.h
)

set(Hyperion_Dispmanx_SOURCES
	hyperion-dispmanx.cpp
	DispmanxWrapper.cpp
)

add_executable( ${PROJECT_NAME}
	${Hyperion_Dispmanx_HEADERS}
	${Hyperion_Dispmanx_SOURCES}
)

target_link_libraries( ${PROJECT_NAME}
	commandline
	hyperion-utils
	flatbufconnect
	flatbuffers
	dispmanx-grabber
	${Dispmanx_LIBRARIES}
	Qt${QT_VERSION_MAJOR}::Core
	Qt${QT_VERSION_MAJOR}::Network
	Qt${QT_VERSION_MAJOR}::Widgets
)

if(ENABLE_MDNS)
        target_link_libraries(${PROJECT_NAME} mdns)
else()
        target_link_libraries(${PROJECT_NAME} ssdp)
endif()

install ( TARGETS ${PROJECT_NAME} DESTINATION "share/hyperion/bin" COMPONENT "hyperion_dispmanx" )

if(CMAKE_HOST_UNIX)
	install(CODE "EXECUTE_PROCESS(COMMAND ln -sf \"../share/hyperion/bin/${PROJECT_NAME}\" \"${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}\" )" COMPONENT  "hyperion_dispmanx" )
	install(FILES "${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}" DESTINATION "bin" RENAME "${PROJECT_NAME}" COMPONENT "hyperion_dispmanx" )
	install(CODE "FILE (REMOVE ${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME} )" COMPONENT  "hyperion_dispmanx" )
endif(CMAKE_HOST_UNIX)
