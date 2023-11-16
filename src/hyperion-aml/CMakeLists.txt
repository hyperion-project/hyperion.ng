cmake_minimum_required(VERSION 3.5.0)
project(hyperion-aml)

add_executable(${PROJECT_NAME}
	AmlogicWrapper.h
	AmlogicWrapper.cpp
	hyperion-aml.cpp
)

target_link_libraries(${PROJECT_NAME}
	commandline
	hyperion-utils
	flatbufconnect
	flatbuffers
	amlogic-grabber
	framebuffer-grabber
	Qt${QT_VERSION_MAJOR}::Widgets
)

if(ENABLE_MDNS)
	target_link_libraries(${PROJECT_NAME} mdns)
else()
	target_link_libraries(${PROJECT_NAME} ssdp)
endif()

if(ENABLE_AMLOGIC)
	target_link_libraries(${PROJECT_NAME} pcre16 dl z)
endif()

install (TARGETS ${PROJECT_NAME} DESTINATION "share/hyperion/bin" COMPONENT "hyperion_aml")

if(CMAKE_HOST_UNIX)
	install(CODE "execute_process(COMMAND ln -sf \"../share/hyperion/bin/${PROJECT_NAME}\" \"${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}\")" COMPONENT "hyperion_aml")
	install(FILES "${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}" DESTINATION "bin" RENAME "${PROJECT_NAME}" COMPONENT "hyperion_aml")
	install(CODE "file (REMOVE ${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME})" COMPONENT "hyperion_aml")
endif(CMAKE_HOST_UNIX)
