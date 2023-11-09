# Define the current source/header locations
SET(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/events)
SET(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/events)

add_library(events
	${CURRENT_HEADER_DIR}/Event.h
	${CURRENT_HEADER_DIR}/EventHandler.h
	${CURRENT_SOURCE_DIR}/EventHandler.cpp
	${CURRENT_HEADER_DIR}/OsEventHandler.h
	${CURRENT_SOURCE_DIR}/OsEventHandler.cpp
)

if (UNIX)
	find_package(Qt${QT_VERSION_MAJOR} COMPONENTS DBus QUIET )
	if (Qt${QT_VERSION_MAJOR}DBus_FOUND)
		target_link_libraries(events Qt${QT_VERSION_MAJOR}::DBus)
		if (NOT APPLE)
			target_compile_definitions(events PUBLIC HYPERION_HAS_DBUS)
		endif()
	endif()
endif(UNIX)

target_include_directories(events PUBLIC
	${CURRENT_HEADER_DIR}
)

target_link_libraries(events
	Qt${QT_VERSION_MAJOR}::Core
	Qt${QT_VERSION_MAJOR}::Widgets
)
