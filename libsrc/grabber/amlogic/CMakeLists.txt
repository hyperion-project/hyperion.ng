add_library(amlogic-grabber
	${CMAKE_SOURCE_DIR}/include/grabber/amlogic/AmlogicGrabber.h
	${CMAKE_SOURCE_DIR}/include/grabber/amlogic/AmlogicWrapper.h
	${CMAKE_SOURCE_DIR}/libsrc/grabber/amlogic/AmlogicGrabber.cpp
	${CMAKE_SOURCE_DIR}/libsrc/grabber/amlogic/AmlogicWrapper.cpp
	${CMAKE_SOURCE_DIR}/libsrc/grabber/amlogic/Amvideocap.h
	${CMAKE_SOURCE_DIR}/libsrc/grabber/amlogic/ion.h
	${CMAKE_SOURCE_DIR}/libsrc/grabber/amlogic/meson_ion.h
	${CMAKE_SOURCE_DIR}/libsrc/grabber/amlogic/IonBuffer.h
	${CMAKE_SOURCE_DIR}/libsrc/grabber/amlogic/IonBuffer.cpp
)

target_link_libraries(amlogic-grabber
	hyperion
	drm-grabber
)
