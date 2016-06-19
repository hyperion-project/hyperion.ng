cmake_minimum_required(VERSION 2.8)
project(hyperion-aml)

find_package(Qt5Widgets REQUIRED)

include_directories(
	${CMAKE_CURRENT_BINARY_DIR}/../../libsrc/protoserver
	${QT_INCLUDES}
	${PROTOBUF_INCLUDE_DIRS}
)

set(Hyperion_AML_QT_HEADERS
	AmlogicWrapper.h
)

set(Hyperion_AML_HEADERS
)

set(Hyperion_AML_SOURCES
	hyperion-aml.cpp
	AmlogicWrapper.cpp
)

QT5_WRAP_CPP(Hyperion_AML_HEADERS_MOC ${Hyperion_AML_QT_HEADERS})

add_executable(${PROJECT_NAME}
	${Hyperion_AML_HEADERS}
	${Hyperion_AML_SOURCES}
	${Hyperion_AML_HEADERS_MOC}
)

target_link_libraries(${PROJECT_NAME}
	getoptPlusPlus
	blackborder
	hyperion-utils
	protoserver
	amlogic-grabber
	pthread
)

qt5_use_modules(${PROJECT_NAME} Widgets Core Gui Network)

install ( TARGETS ${PROJECT_NAME} DESTINATION "bin" COMPONENT ambilight )
