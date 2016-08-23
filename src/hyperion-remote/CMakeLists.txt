cmake_minimum_required(VERSION 2.8)
project(hyperion-remote)

find_package(Qt5 COMPONENTS Core Gui Widgets Network REQUIRED)
#	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}    ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
#	set(CMAKE_CXX_FLAGS "-fPIC")

# The following I do not undrstand completely...
# libQtCore.so uses some hardcoded library path inside which are incorrect after copying the file RPi file system
# Therefore, an extra path is needed on which to find the required libraries
IF ( EXISTS ${CMAKE_FIND_ROOT_PATH}/lib/arm-linux-gnueabihf )
	LINK_DIRECTORIES(${LINK_DIRECTORIES} ${CMAKE_FIND_ROOT_PATH}/lib/arm-linux-gnueabihf)
ENDIF()

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

qt5_use_modules(${PROJECT_NAME} Gui Core Network)

install ( TARGETS ${PROJECT_NAME} DESTINATION "share/hyperion/bin/" COMPONENT "${PLATFORM}" )
