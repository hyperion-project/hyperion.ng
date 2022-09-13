
# With Docker
If you are using [Docker](https://www.docker.com/), you can compile Hyperion inside a docker container. This keeps your system clean and with a simple script it's easy to use. Supported is also cross compiling for Raspberry Pi (Debian Stretch or higher). To compile Hyperion just execute one of the following commands.

The compiled binaries and packages will be available at the deploy folder next to the script.<br/>
Note: call the script with `./docker-compile.sh -h` for more options.

## Native compilation on Raspberry Pi for:

**Raspbian Stretch**
```console
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh -i rpi-raspbian -t stretch
```
**Raspbian Buster/Raspberry Pi OS**
```console
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh -i rpi-raspbian -t buster
```
**Raspberry Pi OS Bullseye**
```console
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh -i rpi-raspbian
```

## Cross compilation on x86_64 for:

**x86_64 (Debian Stretch):**
```console
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh -i x86_64 -t stretch
```
**x86_64 (Debian Buster):**
```console
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh -i x86_64 -t buster
```
**x86_64 (Debian Bullseye):**
```console
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh -i x86_64
```
**Raspberry Pi v1 & ZERO (Debian Stretch)**
```console
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh -i armv6l -t stretch
```
**Raspberry Pi v1 & ZERO (Debian Buster)**
```console
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh -i armv6l -t buster
```
**Raspberry Pi v1 & ZERO (Debian Bullseye)**
```console
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh -i armv6l
```
**Raspberry Pi 2/3/4 (Debian Stretch)**
```console
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh -i armv7l -t stretch
```
**Raspberry Pi 2/3/4 (Debian Buster)**
```console
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh -i armv7l -t buster
```
**Raspberry Pi 2/3/4 (Debian Bullseye)**
```console
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh -i armv7l
```
## Cross compilation on x86_64 for developers
Using additional options you can cross compile locally
-l: use a local hyperion source code directory rather than cloning from GitHub
-c: do incremental compiles, Note: you need to keep the image and tag stable

**Compile code in $HYPERION_HOME incrementally for Raspberry Pi 2/3/4 (Debian Bullseye)**
```console
cd $HYPERION_HOME
./bin/scripts/docker-compile.sh -l -c -i armv7l -t bullseye
```
# The usual way

## Debian/Ubuntu/Win10LinuxSubsystem

**For Linux/Ubuntu(<= 21.10) - Qt5 based**

```console
sudo apt-get update
sudo apt-get install git cmake build-essential qtbase5-dev libqt5serialport5-dev libqt5sql5-sqlite libqt5svg5-dev libqt5x11extras5-dev libusb-1.0-0-dev python3-dev libturbojpeg0-dev libssl-dev
```

**Ubuntu (22.04+) - Qt6 based**

```console
sudo apt-get update
sudo apt-get install git cmake build-essential qt6-base-dev libqt6serialport6-dev libvulkan-dev libgl1-mesa-dev libusb-1.0-0-dev python3-dev libturbojpeg0-dev libssl-dev pkg-config
```

**For Linux X11/XCB grabber support**

```console
sudo apt-get install libxrandr-dev libxrender-dev libxcb-image0-dev libxcb-util0-dev libxcb-shm0-dev libxcb-render0-dev libxcb-randr0-dev
```

**For Linux CEC support**

```console
sudo apt-get install libcec-dev libp8-platform-dev libudev-dev
```

**on RPI you need the videocore IV headers**

```console
sudo apt-get install libraspberrypi-dev
```

**OSMC on Raspberry Pi**
```console
sudo apt-get install rbp-userland-dev-osmc
```

**Additionally for QT6 when QT6 installed separately on Ubuntu < 22.04**
```console
sudo apt-get install postgresql unixodbc libxkbcommon-dev
```

**ATTENTION Win10LinuxSubsystem** we do not (/we can't) support using hyperion in linux subsystem of MS Windows 10, albeit some users tested it with success. Keep in mind to disable
all linux specific led and grabber hardware via cmake. Because we use QT as framework in hyperion, serialport leds and network driven devices could work.


## Arch
See [AUR](https://aur.archlinux.org/packages/?O=0&SeB=nd&K=hyperion&outdated=&SB=n&SO=a&PP=50&do_Search=Go) for PKGBUILDs on arch. If the PKGBUILD does not work ask questions there please.

## Fedora
The following dependencies are needed to build hyperion.ng on fedora.
```console
sudo dnf -y groupinstall "Development Tools"
sudo dnf install python3-devel qt-devel qt5-qtbase-devel qt5-qtserialport-devel xrandr xcb-util-image-devel qt5-qtx11extras-devel turbojpeg-devel libusb-devel xcb-util-devel dbus-devel openssl-devel fedora-packager rpmdevtools gcc libcec-devel
```
After installing the dependencies, you can continue with the compile instructions later on this page (the more detailed way..).

## OSX
To install on OS X you either need Homebrew or Macport but Homebrew is the recommended way to install the packages. To use Homebrew XCode is required as well, use `brew doctor` to check your install.

First you need to install the dependencies:
```console
brew install qt5 python3 cmake libusb doxygen
```

## Windows
We assume a 64bit Windows 10. Install the following;
- [Git](https://git-scm.com/downloads) (Check: Add to PATH)
- [CMake (Windows win64-x64 installer)](https://cmake.org/download/) (Check: Add to PATH)
- [Visual Studio 2022 Community Edition](https://visualstudio.microsoft.com/downloads/#visual-studio-community-2022)
  - Select 'Desktop development with C++'
  - On the right, just select `MSVC v143 VS 2022 C++ x64/x86-Buildtools` and latest `Windows 10 SDK`. Everything else is not needed.
- [Win64 OpenSSL v1.1.1k](https://slproweb.com/products/Win32OpenSSL.html) ([direct link](https://slproweb.com/download/Win64OpenSSL-1_1_1k.exe))
- [Python 3 (Windows x86-64 executable installer)](https://www.python.org/downloads/windows/) (Check: Add to PATH and Debug Symbols)
  - Open a console window and execute `pip install aqtinstall`.
  - Now we can download Qt to _C:\Qt_ `mkdir c:\Qt && aqt install -O c:\Qt 5.15.2 windows desktop win64_msvc2019_64`
  - QT6.2 requires the [Vulkan SDK](https://vulkan.lunarg.com/sdk/home) to be installed
- [libjpeg-turbo SDK for Visual C++](https://sourceforge.net/projects/libjpeg-turbo/files/)
  - Download the latest 64bit installer (currently `libjpeg-turbo-2.1.3-vc64.exe`) and install to its default location `C:\libjpeg-turbo64`.

###  Optional:
- For DirectX9 grabber:
  - [DirectX Software Development Kit](https://www.microsoft.com/en-us/download/details.aspx?id=6812) ([direct link](https://download.microsoft.com/download/A/E/7/AE743F1F-632B-4809-87A9-AA1BB3458E31/DXSDK_Jun10.exe))
- For package creation:
  - [NSIS 3.x](https://sourceforge.net/projects/nsis/files/NSIS%203/) ([direct link](https://sourceforge.net/projects/nsis/files/latest/download))

# Compiling and installing Hyperion

## The general quick way (without big comments)

**complete automated process for Mac/Linux:**
```console
wget -qO- https://raw.githubusercontent.com/hyperion-project/hyperion.ng/master/bin/compile.sh | sh
```

**some more detailed way: (or more or less the content of the script above)**

```console
# be sure you fulfill the prerequisites above
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

In case you would like to build with a dedicated Qt version, Either supply ``QTDIR`` as ``-DQTDIR=<path>`` to cmake or set and environment variable ``QTDIR`` pointing to the Qt installation.

On Windows MSVC2022 set it via the CMakeSettings.json:
```posh
  "configurations": [
    {
      ...
      "environments": [
        {
          "QTDIR": "C:/Qt/6.2.2/msvc2019_64/"
        }
      ]
    },
```

## The detailed way (with many comments)

**Download:**
 Creates hyperion directory and checkout the code from github
```console
export HYPERION_DIR="hyperion"
git clone --recursive --depth 1 https://github.com/hyperion-project/hyperion.ng.git "$HYPERION_DIR"
```

**Preparations:**
Change into hyperion folder and create a build folder
```console
cd "$HYPERION_DIR"
mkdir build
cd build
```

**Generate the make files:**
To generate make files with automatic platform detection and default settings:
This should fit to *RPI, x86, amlogic/wetek:
```console
cmake -DCMAKE_BUILD_TYPE=Release ..
```

*Developers on x86* linux should use:
```console
cmake -DPLATFORM=x11-dev -DCMAKE_BUILD_TYPE=Release ..
```

To use framebuffer instead of dispmanx (for example on the *cubox-i*):
```console
cmake -DENABLE_FB=ON -DCMAKE_BUILD_TYPE=Release ..
```

To generate make files on OS X:

Platform should be auto detected and refer to osx, you can also force osx:
```console
cmake -DPLATFORM=osx -DCMAKE_BUILD_TYPE=Release ..
```

To generate files on Windows (Release+Debug capable):

Platform should be auto detected and refer to windows, you can also force windows:

```posh
# You might need to setup MSVC env first
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
cmake -DPLATFORM=windows -G "Visual Studio 17 2022" ..
```

**Run make to build Hyperion:**
The `-j $(nproc)` specifies the amount of CPU cores to use.
```console
make -j $(nproc)
```

On a mac you can use ``sysctl -n hw.ncpu`` to get the number of available CPU cores to use.

```console
make -j $(sysctl -n hw.ncpu)
```

On Windows run:
```posh
cmake --build . --config Release -- -maxcpucount
```
Maintainer: To build installer, install [NSIS](https://nsis.sourceforge.io/Main_Page) and set env `VCINSTALLDIR="C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC"`

**Install hyperion into your system:**
Copy all necessary files to ``/usr/local/share/hyperion``
```console
sudo make install/strip
```

If you want to install into another location call this before installing

```console
cmake -DCMAKE_INSTALL_PREFIX=/home/pi/apps ..
```
This will install to ``/home/pi/apps/share/hyperion``


