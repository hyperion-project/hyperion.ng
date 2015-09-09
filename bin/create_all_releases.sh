#!/bin/sh

# Create the x64 build
mkdir build-x64
cd build-x64
cmake -DENABLE_DISPMANX=OFF -DENABLE_X11=ON ..
make -j 4
cd ..

# Create the x32 build
mkdir build-x32
cd build-x32
cmake -DIMPORT_PROTOC=../build-x64/protoc_export.cmake -DENABLE_DISPMANX=OFF -DENABLE_X11=ON ..
make -j 4
cd ..

# Create the RPI build
mkdir build-rpi
cd build-rpi
cmake -DCMAKE_TOOLCHAIN_FILE="../Toolchain-rpi.cmake" -DIMPORT_PROTOC=../build-x64/protoc_export.cmake ..
make -j 4
cd ..

# Create the WETEK build
mkdir build-wetek
cd build-wetek
cmake -DCMAKE_TOOLCHAIN_FILE="../Toolchain-rpi.cmake" -DIMPORT_PROTOC=../build-x64/protoc_export.cmake -DENABLE_DISPMANX=OFF -DENABLE_FB=ON -DENABLE_AMLOGIC=ON ..
make -j 4
cd ..

# Create the IMX6 build
mkdir build-imx6
cd build-imx6
cmake -DCMAKE_TOOLCHAIN_FILE="../Toolchain-imx6.cmake" -DIMPORT_PROTOC=../build-x64/protoc_export.cmake -DENABLE_DISPMANX=OFF -DENABLE_FB=ON ..
make -j 4
cd ..

bin/create_release.sh . x64
bin/create_release.sh . x32
bin/create_release.sh . rpi
bin/create_release.sh . wetek
bin/create_release.sh . imx6

