# With Docker
If you are using [Docker](https://www.docker.com/), you can compile Hyperion inside a docker container. This keeps your system clean and with a simple script it's easy to use. Supported is also cross compiling for Raspberry Pi (Debian Stretch or higher). To compile Hyperion just execute one of the following commands.

The compiled binaries and packages will be available at the deploy folder next to the script.<br/>
Note: call the script with `./docker-compile.sh -h` for more options.

## Native compilation on Raspberry Pi for:

**Raspbian Stretch**
```
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh -i rpi-raspbian
```
**Raspbian Buster**
```
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh -i rpi-raspbian -t buster
```

## Cross compilation on x86_64 for:

**x86_64 (Debian Stretch):**
```
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh -i x86_64
```
**x86_64 (Debian Buster):**
```
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh -i x86_64 -t buster
```
**Raspberry Pi v1 & ZERO (Debian Stretch)**
```
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh -i armv6l
```
**Raspberry Pi v1 & ZERO (Debian Buster)**
```
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh -i armv6l -t buster
```
**Raspberry Pi 2/3/4 (Debian Stretch)**
```
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh -i armv7l
```
**Raspberry Pi 2/3/4 (Debian Buster)**
```
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh -i armv7l -t buster
```

# The usual way

## Debian/Ubuntu/Win10LinuxSubsystem

```
sudo apt-get update
sudo apt-get install git cmake build-essential qtbase5-dev libqt5serialport5-dev libqt5sql5-sqlite libqt5x11extras5-dev libusb-1.0-0-dev python3-dev libcec-dev libxcb-image0-dev libxcb-util0-dev libxcb-shm0-dev libxcb-render0-dev libxcb-randr0-dev libxrandr-dev libxrender-dev libavahi-core-dev libavahi-compat-libdnssd-dev libjpeg-dev libturbojpeg0-dev libssl-dev zlib1g-dev
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
brew install zlib
```

## Windows (WIP)
We assume a 64bit Windows 7 or higher. Install the following
- [Git](https://git-scm.com/downloads) (Check: Add to PATH)
- [Python 3 (Windows x86-64 executable installer)](https://www.python.org/downloads/windows/) (Check: Add to PATH and Debug Symbols)
  - Open a console window and execute `pip install aqtinstall`.
  - Now we can download Qt to _C:\Qt_ `mkdir c:\Qt && aqt install -O c:\Qt 5.15.0 windows desktop win64_msvc2019_64`
- [CMake (Windows win64-x64 Installer)](https://cmake.org/download/) (Check: Add to PATH)
- [Win64 OpenSSL v1.1.1g](https://slproweb.com/products/Win32OpenSSL.html) ([direct link](https://slproweb.com/download/Win64OpenSSL-1_1_1g.exe))
- [Visual Studio 2019 Build Tools](https://go.microsoft.com/fwlink/?linkid=840931) ([direct link](https://aka.ms/vs/16/release/vs_buildtools.exe))
  - Select C++ Buildtools
  - On the right, just select `MSVC v142 VS 2019 C++ x64/x86-Buildtools` and latest `Windows 10 SDK`. Everything else is not needed.

# Compiling and installing Hyperion

### The general quick way (without big comments)

complete automated process for Mac/Linux:
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
if this get stucked and dmesg says out of memory try:
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

To generate files on Windows (Release+Debug capable):

Platform should be auto detected and refer to windows, you can also force windows:

```sh
# You might need to setup MSVC env first
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
cmake -DPLATFORM=windows -G "Visual Studio 16 2019" ..
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

On Windows run
```bash
cmake --build . --config Release -- -maxcpucount
```
Maintainer: To build installer, install [NSIS](https://nsis.sourceforge.io/Main_Page) and set env `VCINSTALLDIR="C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC"`

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
