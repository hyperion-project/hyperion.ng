cmake_minimum_required(VERSION 2.8)
project(hyperion-remote)

find_package(Qt4 REQUIRED QtCore QtNetwork)

# The following I do not undrstand completely...
# libQtCore.so uses some hardcoded library path inside which are incorrect after copying the file RPi file system
# Therefor, an extra path is needed on which to find the required libraries
LINK_DIRECTORIES(${LINK_DIRECTORIES} ${CMAKE_FIND_ROOT_PATH}/lib/arm-linux-gnueabihf)

include_directories(${QT_INCLUDE_DIR})

set(hyperion-remote_HEADERS)

set(hyperion-remote_SOURCES
    hyperion-remote.cpp)

qt4_wrap_cpp(HYPERION_REMOTE_MOC_SOURCES
    ${hyperion-remote_HEADERS})

add_executable(hyperion-remote
    ${hyperion-remote_HEADERS}
    ${hyperion-remote_SOURCES}
    ${HYPERION_REMOTE_MOC_SOURCES})

qt4_use_modules(hyperion-remote
    Core
    Network)

target_link_libraries(hyperion-remote
    hyperion-utils
    getoptPlusPlus)
