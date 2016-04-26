#!/bin/sh
# create all directly for release with -DCMAKE_BUILD_TYPE=Release -Wno-dev
# Create the x64 build
mkdir build-x86x64
cd build-x86x64
cmake -DENABLE_DISPMANX=OFF -DENABLE_X11=ON -DCMAKE_BUILD_TYPE=Release -Wno-dev ..
make -j 4
cd ..

# Create the x32 build
#mkdir build-x32
#cd build-x32
#cmake -DIMPORT_PROTOC=../build-x64/protoc_export.cmake -DENABLE_DISPMANX=OFF -DENABLE_X11=ON -DCMAKE_BUILD_TYPE=Release -Wno-dev ..
#make -j 4
#cd ..

# Create the RPI build
mkdir build-rpi
cd build-rpi
cmake -DCMAKE_TOOLCHAIN_FILE="../Toolchain-rpi.cmake" -DIMPORT_PROTOC=../build-x86x64/protoc_export.cmake -DENABLE_WS2812BPWM=ON -DENABLE_WS281XPWM=ON -DCMAKE_BUILD_TYPE=Release -Wno-dev ..
make -j 4
cd ..

# Create the WETEK build
mkdir build-wetek
cd build-wetek
cmake -DCMAKE_TOOLCHAIN_FILE="../Toolchain-rpi.cmake" -DIMPORT_PROTOC=../build-x86x64/protoc_export.cmake -DENABLE_DISPMANX=OFF -DENABLE_FB=ON -DENABLE_AMLOGIC=ON -DCMAKE_BUILD_TYPE=Release -Wno-dev ..
make -j 4
cd ..

# Create the IMX6 build
#mkdir build-imx6
#cd build-imx6
#cmake -DCMAKE_TOOLCHAIN_FILE="../Toolchain-imx6.cmake" -DIMPORT_PROTOC=../build-x32x64/protoc_export.cmake -DENABLE_DISPMANX=OFF -DENABLE_FB=ON -DCMAKE_BUILD_TYPE=Release -Wno-dev ..
#make -j 4
#cd ..

bin/create_release.sh . x86x64
#bin/create_release.sh . x32
bin/create_release.sh . rpi
bin/create_release.sh . wetek
#bin/create_release.sh . imx6

