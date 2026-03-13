cmake_minimum_required(VERSION 3.10.0)
project(hyperion-x11)

if(CMAKE_SYSTEM MATCHES "Darwin")
	include_directories("/opt/X11/include")
endif()

add_executable(${PROJECT_NAME}
	X11Wrapper.h
	X11Wrapper.cpp
	hyperion-x11.cpp
)

target_link_libraries(${PROJECT_NAME}
	commandline
	flatbufconnect
	x11-grabber
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

install(TARGETS ${PROJECT_NAME} DESTINATION "share/hyperion/bin" COMPONENT "hyperion_x11" OPTIONAL)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
	install(CODE "execute_process(COMMAND ln -sf \"../share/hyperion/bin/${PROJECT_NAME}\" \"${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}\")" COMPONENT "hyperion_x11")
	install(FILES "${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}" DESTINATION "bin" RENAME "${PROJECT_NAME}" COMPONENT "hyperion_x11")
	install(CODE "file (REMOVE ${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME})" COMPONENT "hyperion_x11")
endif()
