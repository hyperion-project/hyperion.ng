
# Define the current source locations
set(CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/bonjour)
set(CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/bonjour)

# Group the headers that go through the MOC compiler
set(Bonjour_QT_HEADERS
		${CURRENT_HEADER_DIR}/bonjourrecord.h
		${CURRENT_HEADER_DIR}/bonjourserviceregister.h
)

set(Bonjour_HEADERS
)

set(Bonjour_SOURCES
		${CURRENT_SOURCE_DIR}/bonjourserviceregister.cpp
)

set(Bonjour_RESOURCES
)

if(ENABLE_QT5)
qt5_wrap_cpp(Bonjour_HEADERS_MOC ${Bonjour_QT_HEADERS})
qt5_add_resources(Bonjour_RESOURCES_RCC ${Bonjour_RESOURCES} OPTIONS "-no-compress")
else(ENABLE_QT5)
qt4_wrap_cpp(Bonjour_HEADERS_MOC ${Bonjour_QT_HEADERS})
qt4_add_resources(Bonjour_RESOURCES_RCC ${Bonjour_RESOURCES} OPTIONS "-no-compress")
endif(ENABLE_QT5)

add_library(bonjour
		${Bonjour_HEADERS}
		${Bonjour_QT_HEADERS}
		${Bonjour_SOURCES}
		${Bonjour_RESOURCES}
		${Bonjour_HEADERS_MOC}
		${Bonjour_RESOURCES_RCC}
)

if(ENABLE_QT5)
qt5_use_modules(bonjour Widgets Network)
endif(ENABLE_QT5)

target_link_libraries(bonjour
		libdns_sd.a
		libavahi-client.a
		libavahi-common.a
		libavahi-core.a
		libavahi-qt4.a
		libdbus-1.a
		hyperion
		hyperion-utils
        ${QT_LIBRARIES})
