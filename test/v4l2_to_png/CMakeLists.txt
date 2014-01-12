
# find Qt4
find_package(Qt4 REQUIRED QtCore QtGui)

include_directories(${QT_INCLUDES})

add_executable(v4l2_to_png
	v4l2_to_png.cpp
	V4L2Grabber.h
	V4L2Grabber.cpp)

target_link_libraries(v4l2_to_png
	getoptPlusPlus)

qt4_use_modules(v4l2_to_png
	Core
	Gui)
