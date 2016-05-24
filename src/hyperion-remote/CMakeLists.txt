cmake_minimum_required(VERSION 2.8)

project(hyperion-remote)

# find Qt
if(ENABLE_QT5)
	find_package(Qt5 COMPONENTS Core Gui Widgets Network REQUIRED)
#	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}    ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
#	set(CMAKE_CXX_FLAGS "-fPIC")
else()
	find_package(Qt4 REQUIRED QtCore QtGui QtNetwork)
endif()

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

add_executable(${PROJECT_NAME}
	${hyperion-remote_HEADERS}
	${hyperion-remote_SOURCES})

target_link_libraries(${PROJECT_NAME}
	jsoncpp
	getoptPlusPlus
	${QT_LIBRARIES})

if(ENABLE_QT5)
	qt5_use_modules(${PROJECT_NAME} Widgets Core Network)
else()
	qt4_use_modules(${PROJECT_NAME} Core Gui Network )
endif()

install ( TARGETS ${PROJECT_NAME} DESTINATION "bin" COMPONENT ambilight )

