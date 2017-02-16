
# Define the current source locations
set(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/bonjour)
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/bonjour)

# Group the headers that go through the MOC compiler
set(Bonjour_QT_HEADERS
		${CURRENT_HEADER_DIR}/bonjourserviceregister.h
)

set(Bonjour_HEADERS
)

set(Bonjour_SOURCES
		${CURRENT_SOURCE_DIR}/bonjourserviceregister.cpp
)

set(Bonjour_RESOURCES
)

qt5_wrap_cpp(Bonjour_HEADERS_MOC ${Bonjour_QT_HEADERS})
qt5_add_resources(Bonjour_RESOURCES_RCC ${Bonjour_RESOURCES} OPTIONS "-no-compress")

add_library(bonjour
		${Bonjour_HEADERS}
		${Bonjour_QT_HEADERS}
		${Bonjour_SOURCES}
		${Bonjour_RESOURCES}
		${Bonjour_HEADERS_MOC}
		${Bonjour_RESOURCES_RCC}
)

qt5_use_modules(bonjour Network)

target_link_libraries(bonjour
		hyperion
		hyperion-utils
        ${QT_LIBRARIES})

set(USE_SHARED_AVAHI_LIBS ${DEFAULT_USE_SHARED_AVAHI_LIBS} CACHE BOOL "use avahi libraries from system")

if (USE_SHARED_AVAHI_LIBS)
	target_link_libraries(bonjour
		dns_sd
		avahi-client
		avahi-common
		avahi-core
		dbus-1)
elseif (DEFAULT_USE_AVAHI_LIBS)
	target_link_libraries(bonjour
		libdns_sd.a
		libavahi-client.a
		libavahi-common.a
		libavahi-core.a
		dbus-1)
endif()
