cmake_minimum_required(VERSION 3.10.0)
project(hyperion-osx)

add_executable(${PROJECT_NAME}
	OsxWrapper.h
	OsxWrapper.cpp
	OsxGrabberOptions.h
	OsxGrabberCli.h
	OsxGrabberCli.cpp
	OsxGrabberTraits.h
	OsxGrabberTraits.cpp
	hyperion-osx.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE ${GRABBERAPP_INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME}
	commandline
	flatbufconnect
	osx-grabber
	Qt${QT_VERSION_MAJOR}::Widgets
)

if(ENABLE_MDNS)
	target_link_libraries(${PROJECT_NAME} mdns)
else()
	target_link_libraries(${PROJECT_NAME} ssdp)
endif()

target_link_libraries(${PROJECT_NAME}
	hyperion-utils
)

install(TARGETS ${PROJECT_NAME} DESTINATION "." COMPONENT "hyperion_osx" OPTIONAL)
