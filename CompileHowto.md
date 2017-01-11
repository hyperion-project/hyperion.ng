# Install the required tools and dependencies

## Debian/Ubuntu/Win10LinuxSubsystem

```
sudo apt-get update
sudo apt-get install git cmake build-essential qtbase5-dev libqt5serialport5-dev libusb-1.0-0-dev python-dev libxrender-dev libavahi-core-dev libavahi-compat-libdnssd-dev
```

**ATTENTION Win10LinuxSubsystem** we do not (/we can't) support using hyperion in linux subsystem of MS Windows 10, albeit some users tested it with success. Keep in mind to disable
all linux specific led and grabber hardware via cmake. Because we use QT as framework in hyperion, serialport leds and network driven devices could work.

## RPI Only
when you build on the rapberry pi and include the dispmanx grabber (which is the default)
you also need the firmware including headers installed. This downloads the firmware from the raspberrypi github
and copies the required files to the correct place. The firmware directory can be deleted afterwards if desired.

```
export FIRMWARE_DIR="raspberrypi-firmware"
git clone --depth 1 https://github.com/raspberrypi/firmware.git "$FIRMWARE_DIR"
sudo cp -R "$FIRMWARE_DIR/hardfp/opt/" /opt
```

## Arch
See [AUR](https://aur.archlinux.org/packages/?O=0&SeB=nd&K=hyperion&outdated=&SB=n&SO=a&PP=50&do_Search=Go) for PKGBUILDs on arch. If the PKGBUILD does not work ask questions there please.


## OSX
To install on OS X you either need Homebrew or Macport but Homebrew is the recommended way to install the packages. To use Homebrew XCode is required as well, use `brew doctor` to check your install.

First you need to install the dependencies:
```
brew install qt5
brew install cmake
brew install libusb
brew install doxygen
```


# Compiling and installing Hyperion

### The general quick way (without big comments)
be sure you fullfill the prerequisites above.

assume your home is /home/pi
```bash
git clone --recursive https://github.com/hyperion-project/hyperion.ng.git hyperion
cd hyperion
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j $(nproc)
# optional: install into your system
make install/strip
# to uninstall (not very well tested, please keep that in mind)
make uninstall
# ... or run it from compile directory
bin/hyperiond myconfig.json
# webui is located on localhost:8099
```


### Download
 Create hyperion directory and checkout the code from github

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

### Preparations
Change into hyperion folder and create a build folder
```
cd "$HYPERION_DIR"
mkdir build
cd build
```

### Generate the make files:

To generate make files with automatic platform detection and default settings:

This should fit to *RPI, x86, amlogic/wetek*
```
cmake -DCMAKE_BUILD_TYPE=Release ..
```

*Developers on x86* linux should use:
```
cmake -DPLATFORM=x86-dev -DCMAKE_BUILD_TYPE=Release ..
```

To use framebuffer instead of dispmanx (for example on the *cubox-i*):
```
cmake -DENABLE_FB=ON -DCMAKE_BUILD_TYPE=Release ..
```

To generate make files on OS X:

After which you can run cmake with the correct qt5 path:
```
cmake -DCMAKE_PREFIX_PATH=/usr/local/Cellar/qt5/5.7.0  -DCMAKE_BUILD_TYPE=Release ..
```

### Run make to build Hyperion
The `-j $(nproc)` specifies the amount of CPU cores to use.
```bash
make -j $(nproc)
```

On a mac you can use ``sysctl -n hw.ncpu`` to get the number of available CPU cores to use.

```bash
make -j $(sysctl -n hw.ncpu)
```

### Install hyperion into your system

Copy all necessary files to ``/usr/local/share/hyperion``
```bash
sudo make install/strip
```

If you want to install into another location call this before installing

```bash
cmake -DCMAKE_INSTALL_PREFIX=/home/pi/apps ..
```
This will install to ``/home/pi/apps/share/hyperion``


### Integrating hyperion into your system

... ToDo


