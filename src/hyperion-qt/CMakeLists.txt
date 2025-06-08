cmake_minimum_required(VERSION 3.10.0)
project(hyperion-qt)

add_executable(${PROJECT_NAME}
	QtWrapper.h
	QtWrapper.cpp
	hyperion-qt.cpp
	$<$<BOOL:${WIN32}>:${CMAKE_BINARY_DIR}/win.rc>
)

target_link_libraries(${PROJECT_NAME}
	commandline
	qt-grabber
	flatbufconnect
	Qt${QT_VERSION_MAJOR}::Widgets
)

if(ENABLE_MDNS)
	target_link_libraries(${PROJECT_NAME} mdns)
else()
	target_link_libraries(${PROJECT_NAME} ssdp)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	install(TARGETS ${PROJECT_NAME} DESTINATION "." COMPONENT "hyperion_qt" OPTIONAL)
elseif(NOT WIN32)
	install(TARGETS ${PROJECT_NAME} DESTINATION "share/hyperion/bin" COMPONENT "hyperion_qt" OPTIONAL)
else()
	install(TARGETS ${PROJECT_NAME} DESTINATION "bin" COMPONENT "hyperion_qt" OPTIONAL)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
	install(CODE "execute_process(COMMAND ln -sf \"../share/hyperion/bin/${PROJECT_NAME}\" \"${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}\")" COMPONENT  "hyperion_qt")
	install(FILES "${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}" DESTINATION "bin" RENAME "${PROJECT_NAME}" COMPONENT "hyperion_qt")
	install(CODE "file (REMOVE ${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME})" COMPONENT  "hyperion_qt")
endif()
