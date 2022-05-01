cmake_minimum_required(VERSION 3.1.0)
project(hyperion-aml)

find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Network Widgets REQUIRED)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../../libsrc/flatbufserver
	${FLATBUFFERS_INCLUDE_DIRS}
)

set(Hyperion_AML_HEADERS
	AmlogicWrapper.h
)

set(Hyperion_AML_SOURCES
	hyperion-aml.cpp
	AmlogicWrapper.cpp
)

add_executable(${PROJECT_NAME}
	${Hyperion_AML_HEADERS}
	${Hyperion_AML_SOURCES}
)

target_link_libraries(${PROJECT_NAME}
	commandline
	hyperion-utils
	flatbufconnect
	flatbuffers
	amlogic-grabber
	framebuffer-grabber
	Qt${QT_VERSION_MAJOR}::Core
	Qt${QT_VERSION_MAJOR}::Network
	Qt${QT_VERSION_MAJOR}::Widgets
)

if(ENABLE_MDNS)
        target_link_libraries(${PROJECT_NAME} mdns)
else()
        target_link_libraries(${PROJECT_NAME} ssdp)
endif()

if (ENABLE_AMLOGIC)
	target_link_libraries(${PROJECT_NAME}
		pcre16 dl z
	)
endif()

install ( TARGETS ${PROJECT_NAME} DESTINATION "share/hyperion/bin" COMPONENT "hyperion_aml" )

if(CMAKE_HOST_UNIX)
	install(CODE "EXECUTE_PROCESS(COMMAND ln -sf \"../share/hyperion/bin/${PROJECT_NAME}\" \"${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}\" )" COMPONENT "hyperion_aml" )
	install(FILES "${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}" DESTINATION "bin" RENAME "${PROJECT_NAME}" COMPONENT "hyperion_aml" )
	install(CODE "FILE (REMOVE ${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME} )" COMPONENT "hyperion_aml" )
endif(CMAKE_HOST_UNIX)
