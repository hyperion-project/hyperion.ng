cmake_minimum_required(VERSION 3.5.0)
project(hyperion-v4l2)

add_executable(${PROJECT_NAME}
	ScreenshotHandler.h
	ScreenshotHandler.cpp
	hyperion-v4l2.cpp
)

target_link_libraries(${PROJECT_NAME}
	v4l2-grabber
	commandline
	hyperion-utils
	flatbufconnect
	flatbuffers
	Qt${QT_VERSION_MAJOR}::Widgets
)

if(ENABLE_MDNS)
	target_link_libraries(${PROJECT_NAME} mdns)
else()
	target_link_libraries(${PROJECT_NAME} ssdp)
endif()

install (TARGETS ${PROJECT_NAME} DESTINATION "share/hyperion/bin" COMPONENT "hyperion_v4l2")

if(CMAKE_HOST_UNIX)
	install(CODE "execute_process(COMMAND ln -sf \"../share/hyperion/bin/${PROJECT_NAME}\" \"${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}\")" COMPONENT "hyperion_v4l2")
	install(FILES "${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}" DESTINATION "bin" RENAME "${PROJECT_NAME}" COMPONENT "hyperion_v4l2")
	install(CODE "file (REMOVE ${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME})" COMPONENT "hyperion_v4l2")
endif(CMAKE_HOST_UNIX)
