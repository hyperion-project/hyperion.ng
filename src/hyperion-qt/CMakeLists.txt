cmake_minimum_required(VERSION 3.5.0)
project(hyperion-qt)

if(WIN32)
	# generate windows .rc file for this binary
	string(REPLACE "/" "\\\\" WIN_RC_ICON_PATH ${CMAKE_SOURCE_DIR}/cmake/nsis/installer.ico)
	configure_file(${CMAKE_SOURCE_DIR}/cmake/win/win.rc.in ${CMAKE_BINARY_DIR}/win.rc)
	set(WIN_RC_FILE ${CMAKE_BINARY_DIR}/win.rc)
endif()

add_executable(${PROJECT_NAME}
	QtWrapper.h
	QtWrapper.cpp
	hyperion-qt.cpp
	${WIN_RC_FILE}
)

target_link_libraries(${PROJECT_NAME}
	commandline
	qt-grabber
	flatbufconnect
	flatbuffers
	Qt${QT_VERSION_MAJOR}::Widgets
)

if(ENABLE_MDNS)
	target_link_libraries(${PROJECT_NAME} mdns)
else()
	target_link_libraries(${PROJECT_NAME} ssdp)
endif()

if(APPLE)
	install (TARGETS ${PROJECT_NAME} DESTINATION "." COMPONENT "hyperion_qt")
elseif(NOT WIN32)
	install (TARGETS ${PROJECT_NAME} DESTINATION "share/hyperion/bin" COMPONENT "hyperion_qt")
else()
	install (TARGETS ${PROJECT_NAME} DESTINATION "bin" COMPONENT "hyperion_qt")
endif()

if(CMAKE_HOST_UNIX  AND NOT APPLE)
	install(CODE "execute_process(COMMAND ln -sf \"../share/hyperion/bin/${PROJECT_NAME}\" \"${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}\")" COMPONENT  "hyperion_qt")
	install(FILES "${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME}" DESTINATION "bin" RENAME "${PROJECT_NAME}" COMPONENT "hyperion_qt")
	install(CODE "file (REMOVE ${CMAKE_BINARY_DIR}/symlink_${PROJECT_NAME})" COMPONENT  "hyperion_qt")
endif()
