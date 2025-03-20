add_library(events
	${CMAKE_SOURCE_DIR}/include/events/EventEnum.h
	${CMAKE_SOURCE_DIR}/include/events/EventHandler.h
	${CMAKE_SOURCE_DIR}/include/events/OsEventHandler.h
	${CMAKE_SOURCE_DIR}/include/events/EventScheduler.h
	${CMAKE_SOURCE_DIR}/libsrc/events/EventHandler.cpp
	${CMAKE_SOURCE_DIR}/libsrc/events/OsEventHandler.cpp
	${CMAKE_SOURCE_DIR}/libsrc/events/EventScheduler.cpp
)

if(CMAKE_SYSTEM MATCHES "Linux")
	find_package(Qt${QT_VERSION_MAJOR} COMPONENTS DBus QUIET)
	if(Qt${QT_VERSION_MAJOR}DBus_FOUND)
		target_link_libraries(events Qt${QT_VERSION_MAJOR}::DBus)
		target_compile_definitions(events PRIVATE HYPERION_HAS_DBUS)
	endif()
endif()

target_link_libraries(events
	hyperion-utils
	Qt${QT_VERSION_MAJOR}::Widgets
)

if(CMAKE_SYSTEM MATCHES "Darwin")
	set_source_files_properties(OsEventHandler.cpp PROPERTIES COMPILE_FLAGS "-x objective-c++")
	target_link_libraries(events "-framework AppKit")
endif()
