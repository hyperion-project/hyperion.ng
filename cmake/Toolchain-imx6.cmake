SET(CUBIXCROSS_DIR $ENV{HOME}/hummingboard)

SET(CMAKE_SYSTEM_NAME Linux) 
SET(CMAKE_SYSTEM_VERSION 1) 

# specify the cross compiler 
SET(CMAKE_C_COMPILER   ${CUBIXCROSS_DIR}/gcc-linaro-arm-linux-gnueabihf-4.8-2013.10_linux/bin/arm-linux-gnueabihf-gcc)
SET(CMAKE_CXX_COMPILER ${CUBIXCROSS_DIR}/gcc-linaro-arm-linux-gnueabihf-4.8-2013.10_linux/bin/arm-linux-gnueabihf-g++)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard")
SET(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -march=armv7-a -mcpu=cortex-a9 -mtune=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard")

# where is the target environment 
SET(CMAKE_FIND_ROOT_PATH ${CUBIXCROSS_DIR}/rootfs) 

# search for programs in the build host directories 
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories 
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY) 
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

include_directories(${CMAKE_FIND_ROOT_PATH}/usr/include)
