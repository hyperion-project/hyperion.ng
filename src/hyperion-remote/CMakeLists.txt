cmake_minimum_required(VERSION 2.8)

project(hyperion-remote)

find_package(Qt4 REQUIRED QtCore QtGui QtNetwork)

# The following I do not undrstand completely...
# libQtCore.so uses some hardcoded library path inside which are incorrect after copying the file RPi file system
# Therefor, an extra path is needed on which to find the required libraries
LINK_DIRECTORIES(${LINK_DIRECTORIES} ${CMAKE_FIND_ROOT_PATH}/lib/arm-linux-gnueabihf)

include_directories(${QT_INCLUDES})

set(hyperion-remote_HEADERS
	CustomParameter.h
	JsonConnection.h
	ColorTransformValues.h)

set(hyperion-remote_SOURCES
	hyperion-remote.cpp
	JsonConnection.cpp)

add_executable(hyperion-remote
	${hyperion-remote_HEADERS}
	${hyperion-remote_SOURCES})

qt4_use_modules(hyperion-remote
	Core
	Gui
	Network)

target_link_libraries(hyperion-remote
	hyperion-utils
	getoptPlusPlus)
