cmake_minimum_required(VERSION 2.8)

project(hyperion-remote)

# find Qt4
if(ENABLE_QT5)
find_package(Qt5 COMPONENTS Core Gui Widgets Network REQUIRED)
#  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}    ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
#      set(CMAKE_CXX_FLAGS "-fPIC")
else(ENABLE_QT5)
find_package(Qt4 REQUIRED QtCore QtGui QtNetwork)
endif(ENABLE_QT5)

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

if(ENABLE_QT5)
qt5_use_modules(hyperion-remote Widgets Network)
endif(ENABLE_QT5)

target_link_libraries(hyperion-remote
	jsoncpp
    getoptPlusPlus
    ${QT_LIBRARIES})
