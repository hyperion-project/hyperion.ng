# With Docker
If you are using [Docker](https://www.docker.com/), you can compile Hyperion inside a docker container. This keeps your system clean and with a simple script it's easy to use. Supported is also cross compiling for Raspberry Pi (Debian Stretch or higher). To compile Hyperion just execute one of the following commands.

The compiled binaries and packages will be available at the deploy folder next to the script.<br/>
Note: call the script with `./docker-compile.sh -h` for more options

## Native compiling on Raspberry Pi

**Raspbian Stretch**
```
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh -t rpi-raspbian-stretch
```
**Raspbian Buster**
```
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh -t rpi-raspbian-buster
```

## Cross compiling on X64_86 for:

**X64:**
```
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh
```
**i386:**
```
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh -t i386
```
**Raspberry Pi v1 & ZERO**
```
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh -t armv6hf
```
**Raspberry Pi 2 & 3**
```
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh -t armv7hf
```

# The usual way

## Debian/Ubuntu/Win10LinuxSubsystem

```
sudo apt-get update
sudo apt-get install git cmake build-essential qtbase5-dev libqt5serialport5-dev libusb-1.0-0-dev python3-dev libxrender-dev libavahi-core-dev libavahi-compat-libdnssd-dev libjpeg-dev libqt5sql5-sqlite
```

**on RPI you need the videocore IV headers**

```
sudo apt-get install libraspberrypi-dev
```

**OSMC on Raspberry Pi**
```
sudo apt-get install rbp-userland-dev-osmc
```

**ATTENTION Win10LinuxSubsystem** we do not (/we can't) support using hyperion in linux subsystem of MS Windows 10, albeit some users tested it with success. Keep in mind to disable
all linux specific led and grabber hardware via cmake. Because we use QT as framework in hyperion, serialport leds and network driven devices could work.


## Arch
See [AUR](https://aur.archlinux.org/packages/?O=0&SeB=nd&K=hyperion&outdated=&SB=n&SO=a&PP=50&do_Search=Go) for PKGBUILDs on arch. If the PKGBUILD does not work ask questions there please.


## OSX
To install on OS X you either need Homebrew or Macport but Homebrew is the recommended way to install the packages. To use Homebrew XCode is required as well, use `brew doctor` to check your install.

First you need to install the dependencies:
```
brew install qt5
brew install python3
brew install cmake
brew install libusb
brew install doxygen
```


# Compiling and installing Hyperion

### The general quick way (without big comments)

complete automated process:
```bash
wget -qO- https://raw.githubusercontent.com/hyperion-project/hyperion.ng/master/bin/compile.sh | sh
```

some more detailed way: (or more or less the content of the script above)
be sure you fulfill the prerequisites above.

```bash
git clone --recursive https://github.com/hyperion-project/hyperion.ng.git hyperion
cd hyperion
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j $(nproc)
if this get stucked and dmseg says out of memory try:
make -j 2
# optional: install into your system
sudo make install/strip
# to uninstall (not very well tested, please keep that in mind)
sudo make uninstall
# ... or run it from compile directory
bin/hyperiond
# webui is located on localhost:8090 or 8091
```


### Download
 Creates hyperion directory and checkout the code from github

```
export HYPERION_DIR="hyperion"
git clone --recursive --depth 1 https://github.com/hyperion-project/hyperion.ng.git "$HYPERION_DIR"
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
cmake -DPLATFORM=x11-dev -DCMAKE_BUILD_TYPE=Release ..
```

To use framebuffer instead of dispmanx (for example on the *cubox-i*):
```
cmake -DENABLE_FB=ON -DCMAKE_BUILD_TYPE=Release ..
```

To generate make files on OS X:

Platform should be auto detected and refer to osx, you can also force osx:
```
cmake -DPLATFORM=osx -DCMAKE_BUILD_TYPE=Release ..
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
