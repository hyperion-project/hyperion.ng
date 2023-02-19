# Define the current source locations
SET( CURRENT_HEADER_DIR ${CMAKE_SOURCE_DIR}/include/grabber )
SET( CURRENT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/libsrc/grabber/audio )


if (WIN32)
    FILE ( GLOB AUDIO_GRABBER_SOURCES "${CURRENT_HEADER_DIR}/Audio*Windows.h"  "${CURRENT_HEADER_DIR}/AudioGrabber.h"  "${CURRENT_HEADER_DIR}/AudioWrapper.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*Windows.cpp"  "${CURRENT_SOURCE_DIR}/AudioGrabber.cpp"  "${CURRENT_SOURCE_DIR}/AudioWrapper.cpp")
elseif(${CMAKE_SYSTEM} MATCHES "Linux")
    FILE ( GLOB AUDIO_GRABBER_SOURCES "${CURRENT_HEADER_DIR}/Audio*Linux.h"  "${CURRENT_HEADER_DIR}/AudioGrabber.h"  "${CURRENT_HEADER_DIR}/AudioWrapper.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*Linux.cpp"  "${CURRENT_SOURCE_DIR}/AudioGrabber.cpp"  "${CURRENT_SOURCE_DIR}/AudioWrapper.cpp")
elseif (APPLE)
    #TODO
    #FILE ( GLOB AUDIO_GRABBER_SOURCES "${CURRENT_HEADER_DIR}/Audio*Apple.h"  "${CURRENT_HEADER_DIR}/AudioGrabber.h"  "${CURRENT_HEADER_DIR}/AudioWrapper.h"  "${CURRENT_SOURCE_DIR}/*.h"  "${CURRENT_SOURCE_DIR}/*Apple.cpp"  "${CURRENT_SOURCE_DIR}/AudioGrabber.cpp"  "${CURRENT_SOURCE_DIR}/AudioWrapper.cpp")
endif()

add_library( audio-grabber ${AUDIO_GRABBER_SOURCES} )

set(AUDIO_LIBS hyperion)


if (WIN32)
    set(AUDIO_LIBS ${AUDIO_LIBS} DSound)
elseif(${CMAKE_SYSTEM} MATCHES "Linux")
    find_package(ALSA REQUIRED)                                    
    if (ALSA_FOUND)                                                
        include_directories(${ALSA_INCLUDE_DIRS})
        set(AUDIO_LIBS ${AUDIO_LIBS} ${ALSA_LIBRARIES})
    endif(ALSA_FOUND)

    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    set(AUDIO_LIBS ${AUDIO_LIBS} Threads::Threads) # PRIVATE
endif()


target_link_libraries(audio-grabber ${AUDIO_LIBS} ${QT_LIBRARIES})
