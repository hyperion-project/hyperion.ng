cmake_minimum_required(VERSION 3.10.0)
project(hyperion-remote)

add_executable(${PROJECT_NAME}
	JsonConnection.h
	JsonConnection.cpp
	hyperion-remote.cpp
	$<$<BOOL:${WIN32}>:${CMAKE_BINARY_DIR}/win.rc>
)

target_link_libraries(${PROJECT_NAME}
	commandline
	hyperion-utils
	Qt${QT_VERSION_MAJOR}::Widgets
)

if(ENABLE_MDNS)
	target_link_libraries(${PROJECT_NAME} mdns)
else()
	target_link_libraries(${PROJECT_NAME} ssdp)
endif()

if(ENABLE_EFFECTENGINE)
	target_link_libraries(${PROJECT_NAME} effectengine)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	install(TARGETS ${PROJECT_NAME} DESTINATION "." COMPONENT "hyperion_remote" OPTIONAL)
elseif(NOT WIN32)
	install(TARGETS ${PROJECT_NAME} DESTINATION "share/hyperion/bin" COMPONENT "hyperion_remote" OPTIONAL)
else()
	install(TARGETS ${PROJECT_NAME} DESTINATION "bin" COMPONENT "hyperion_remote" OPTIONAL)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
	install(CODE "execute_process(COMMAND ln -sf \"../share/hyperion/bin/${PROJECT_NAME}\" \"${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}\")" COMPONENT "hyperion_remote")
	install(FILES "${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}" DESTINATION "bin" RENAME "${PROJECT_NAME}" COMPONENT "hyperion_remote")
	install(CODE "file (REMOVE ${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME})" COMPONENT "hyperion_remote")
endif()
