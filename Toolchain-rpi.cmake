SET(RASPCROSS_DIR $ENV{HOME}/raspberrypi)

SET(CMAKE_SYSTEM_NAME Linux) 
SET(CMAKE_SYSTEM_VERSION 1) 

# specify the cross compiler 
SET(CMAKE_C_COMPILER   ${RASPCROSS_DIR}/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/arm-linux-gnueabihf-gcc)
SET(CMAKE_CXX_COMPILER ${RASPCROSS_DIR}/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian-x64/bin/arm-linux-gnueabihf-g++)

# where is the target environment 
SET(CMAKE_FIND_ROOT_PATH ${RASPCROSS_DIR}/rootfs) 

# search for programs in the build host directories 
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# for libraries and headers in the target directories 
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY) 
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

