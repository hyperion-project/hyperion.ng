cmake_minimum_required(VERSION 2.8.12)

SET(Hyperiond-starter_HEADERS
	process.h
	configHandle.h
)

set(Hyperiond-starter_SOURCES
	process.cpp
	configHandle.cpp
	main.cpp
)

add_executable(hyperiond-starter
	${Hyperiond-starter_HEADERS}
	${Hyperiond-starter_SOURCES}
)

target_link_libraries(hyperiond-starter
	hyperion
	Qt5::Core
	Qt5::Network
)

install ( TARGETS hyperiond-starter DESTINATION "share/hyperion/bin/" COMPONENT "${PLATFORM}" )

if(CMAKE_HOST_UNIX)
	install(CODE "EXECUTE_PROCESS(COMMAND ln -sf \"../share/hyperion/bin/hyperiond-starter\" \"${CMAKE_BINARY_DIR}/symlink_hyperiond-starter\" )" COMPONENT  "${PLATFORM}" )
	install(FILES ${CMAKE_BINARY_DIR}/symlink_hyperiond-starter DESTINATION "bin" RENAME hyperiond-starter COMPONENT "${PLATFORM}" )
	install(CODE "FILE (REMOVE ${CMAKE_BINARY_DIR}/symlink_hyperiond-starter )" COMPONENT "${PLATFORM}" )
endif(CMAKE_HOST_UNIX)
