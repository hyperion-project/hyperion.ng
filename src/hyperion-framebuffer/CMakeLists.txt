cmake_minimum_required(VERSION 3.5.0)
project(hyperion-framebuffer)

add_executable(${PROJECT_NAME}
	FramebufferWrapper.h
	FramebufferWrapper.cpp
	hyperion-framebuffer.cpp
)

target_link_libraries(${PROJECT_NAME}
	commandline
	hyperion-utils
	flatbufconnect
	flatbuffers
	framebuffer-grabber
	Qt${QT_VERSION_MAJOR}::Widgets
)

if(ENABLE_MDNS)
	target_link_libraries(${PROJECT_NAME} mdns)
else()
	target_link_libraries(${PROJECT_NAME} ssdp)
endif()

install(TARGETS ${PROJECT_NAME} DESTINATION "share/hyperion/bin" COMPONENT "hyperion_framebuffer" OPTIONAL)

if(CMAKE_HOST_UNIX)
	install(CODE "execute_process(COMMAND ln -sf \"../share/hyperion/bin/${PROJECT_NAME}\" \"${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}\")" COMPONENT  "hyperion_framebuffer")
	install(FILES "${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}" DESTINATION "bin" RENAME "${PROJECT_NAME}" COMPONENT "hyperion_framebuffer")
	install(CODE "file (REMOVE ${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME})" COMPONENT  "hyperion_framebuffer")
endif()
