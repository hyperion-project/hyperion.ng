# Install the required tools and dependencies

```
sudo apt-get update
sudo apt-get install git cmake build-essential qtbase5-dev libqt5serialport5-dev libusb-1.0-0-dev python-dev libxrender-dev libavahi-core-dev libavahi-compat-libdnssd-dev
```

# RPI Only
when you build on the rapberry pi and include the dispmanx grabber (which is the default) 
you also need the firmware including headers installed. This downloads the firmware from the raspberrypi github
and copies the required files to the correct place. The firmware directory can be deleted afterwards if desired.

```
export FIRMWARE_DIR="raspberrypi-firmware"
git clone --depth 1 https://github.com/raspberrypi/firmware.git "$FIRMWARE_DIR"
sudo cp -R "$FIRMWARE_DIR/hardfp/opt/" /opt
```

# Create hyperion directory and checkout the code from github

You might want to add `--depth 1` to the `git` command if you only want to compile the current source and have no need for the entire git repository

```
export HYPERION_DIR="hyperion"
git clone --recursive https://github.com/hyperion-project/hyperion.ng.git "$HYPERION_DIR"
```

**Note:** If you forget the --recursive in above statement or you are updating an existing clone you need to clone the protobuf submodule by runnning the follwing two statements:
```
git submodule init
git submodule update
```

# Create and enter the build directory
```
mkdir "$HYPERION_DIR/build"
cd "$HYPERION_DIR/build"
```

# Generate the make files:

To generate make files on the raspberry pi WITHOUT PWM SUPPORT:
```
cmake -DPLATFORM=rpi -DCMAKE_BUILD_TYPE=Release ..
```

To generate make files on the raspberry pi WITH PWM SUPPORT:
```
cmake -DPLATFORM=rpi-pwm -DCMAKE_BUILD_TYPE=Release ..
```

To generate make files on a regular x86 or amd64 system:
```
cmake -DPLATFORM=x86 -DCMAKE_BUILD_TYPE=Release ..
```

To generate make files on a amlogic system:
```
cmake -DPLATFORM=aml -DCMAKE_BUILD_TYPE=Release ..
```

To use framebuffer instead of dispmanx (for example on the *cubox-i*):
```
cmake -DENABLE_FB=ON -DCMAKE_BUILD_TYPE=Release ..
```

To generate make files on OS X:

To install on OS X you either need Homebrew or Macport but Homebrew is the recommended way to install the packages. To use Homebrew XCode is required as well, use `brew doctor` to check your install.

First you need to install the dependencies:
```
brew install qt5
brew install cmake
brew install libusb
brew install doxygen
```

After which you can run cmake with the correct qt5 path:
```
cmake -DENABLE_V4L2=OFF -DENABLE_OSX=ON -DENABLE_X11=OFF -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_PREFIX_PATH=/usr/local/Cellar/qt5/5.7.0 ..

```

# Run make to build Hyperion
The `-j $(nproc)` specifies the amount of CPU cores to use.
```
make -j $(nproc)
```

On a mac you can use ``sysctl -n hw.ncpu`` to get the number of available CPU cores to use.

```bash
make -j $(sysctl -n hw.ncpu)
``` 


#After compile, to remove any stuff not needed for a release version.
```
strip bin/*
```
# The binaries are build in "$HYPERION_DIR/build/bin". You could copy those to /usr/bin
```
sudo cp ./bin/hyperion-remote /usr/bin/
sudo cp ./bin/hyperiond /usr/bin/
```

On a Mac with Sierra you won't be able to copy these files to the ``/usr/bin/`` folder due to Sierra's file protection. You can copy those files to ``/usr/local/bin`` instead.

```bash
cp ./bin/hyperion-remote /usr/local/bin
cp ./bin/hyperiond /usr/local/bin
```

The better way to do this is to use the make install script, which copies all necessary files to ``/usr/local/share/hyperion``:

```bash
sudo make install
```

# Copy the effect folder (if you did not use the normal installation methode before)
```
sudo mkdir -p /usr/local/share/hyperion/effects && sudo cp -R ../effects/ /usr/local/share/hyperion/effects/
```
