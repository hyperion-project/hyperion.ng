find_package(DirectX9 REQUIRED)

add_library(directx-grabber
	${CMAKE_SOURCE_DIR}/include/grabber/directx/DirectXGrabber.h
	${CMAKE_SOURCE_DIR}/include/grabber/directx/DirectXWrapper.h
	${CMAKE_SOURCE_DIR}/libsrc/grabber/directx/DirectXGrabber.cpp
	${CMAKE_SOURCE_DIR}/libsrc/grabber/directx/DirectXWrapper.cpp
)

target_link_libraries(directx-grabber
	hyperion
	${DIRECTX9_LIBRARIES}
)

target_include_directories(directx-grabber PUBLIC
	${DIRECTX9_INCLUDE_DIRS}
)
