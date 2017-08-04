
# Define the current source locations
set(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/bonjour)
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/bonjour)

FILE ( GLOB Bonjour_SOURCES "${CURRENT_HEADER_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*.cpp" )

add_library(bonjour ${Bonjour_SOURCES} )

target_link_libraries(bonjour
		hyperion
		hyperion-utils
		Qt5::Network
)

IF (NOT APPLE)
	set(USE_SHARED_AVAHI_LIBS ${DEFAULT_USE_SHARED_AVAHI_LIBS} CACHE BOOL "use avahi libraries from system")

	if (USE_SHARED_AVAHI_LIBS)
		target_link_libraries(bonjour
			dns_sd
			avahi-client
			avahi-common
			avahi-core)
	else()
		target_link_libraries(bonjour
			libdns_sd.a
			libavahi-client.a
			libavahi-common.a
			libavahi-core.a)
	endif()
	target_link_libraries(bonjour dbus-1)
ENDIF()
