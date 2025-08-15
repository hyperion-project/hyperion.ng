
# With Docker
If you are using [Docker](https://www.docker.com/), you can compile Hyperion inside a docker container. This keeps your system clean and with a simple script it's easy to use. Supported is also cross compiling for Raspberry Pi (Debian Buster or higher). To compile Hyperion just execute one of the following commands.

The compiled binaries and packages will be available at the deploy folder next to the script.<br/>

> [!NOTE]
> Call the script with `./docker-compile.sh --help` for more options.

## Cross compilation on amd64 (aka x86_64), sample commands

### Debian

**amd64 (Bookworm):**
```console
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh --name bookworm
```
**arm64 or Raspberry Pi 5 (Bookworm)**
```console
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh --architecture arm64 --name bookworm
```
**Raspberry Pi 2/3/4 (Bookworm)**
```console
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh --architecture arm/v7 --name bookworm
```
**Raspberry Pi v1 & ZERO (Bookworm)**
```console
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh --architecture arm/v6 --name bookworm
```

### Ubuntu

**amd64 (Noble Numbat):**
```console
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh --name noble
```

### Fedora

**amd64 (41):**
```console
wget -qN https://raw.github.com/hyperion-project/hyperion.ng/master/bin/scripts/docker-compile.sh && chmod +x *.sh && ./docker-compile.sh --name 41
```

## Cross compilation on amd64 for developers
Using additional options you can cross compile locally
--local: use a local hyperion source code directory rather than cloning from GitHub
--incremental: do incremental compiles, Note: you need to keep the image and tag stable

**Compile code in $HYPERION_HOME incrementally for Raspberry Pi 2/3/4 (Debian Bookworm)**
```console
cd $HYPERION_HOME
./bin/scripts/docker-compile.sh --local --incremental --architecture arm/v7 --name bookworm
```

# The usual way

## Debian/Ubuntu/Win10LinuxSubsystem

**For Linux/Ubuntu(<= 21.10) - Qt5 based**

```console
sudo apt-get update
sudo apt-get install git cmake build-essential ninja-build qtbase5-dev libqt5serialport5-dev libqt5websockets5-dev libqt5sql5-sqlite libqt5svg5-dev libqt5x11extras5-dev libusb-1.0-0-dev python3-dev libasound2-dev libturbojpeg0-dev libjpeg-dev libssl-dev libftdi1-dev
```

**Ubuntu (22.04+) - Qt6 based**

```console
sudo apt-get update
sudo apt-get install git cmake build-essential ninja-build qt6-base-dev libqt6serialport6-dev libqt6websockets6-dev libxkbcommon-dev libvulkan-dev libgl1-mesa-dev libusb-1.0-0-dev python3-dev libasound2-dev libturbojpeg0-dev libjpeg-dev libssl-dev pkg-config libftdi1-dev
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
sudo dnf install ninja-build python3-devel qt-devel qt6-qtbase-devel qt6-qtserialport-devel qt6-qtwebsockets-devel xrandr xcb-util-image-devel qt5-qtx11extras-devel alsa-lib-devel turbojpeg-devel libusb-devel xcb-util-devel dbus-devel openssl-devel fedora-packager rpmdevtools gcc libcec-devel libftdi1-dev
```
After installing the dependencies, you can continue with the compile instructions later on this page (the more detailed way..).

## macOS
To install on OS X you either need [Homebrew](https://brew.sh/) or [Macport](https://www.macports.org/) but Homebrew is the recommended way to install the packages. To use Homebrew, XCode is required as well, use `brew doctor` to check your install.

First you need to install the dependencies for either the QT5 or QT6 build:
#### QT5
```console
brew install git qt@5 ninja python3 cmake libusb openssl@1.1 libftdi pkg-config
```
#### QT6
```console
brew install git qt ninja python3 cmake libusb openssl@3 libftdi pkg-config
```

## Windows
> [!NOTE]
> When downloading, please remember whether you have an x64 or an ARM64 architecture.

We assume a 64bit Windows 11. Install the following:
- [Git](https://git-scm.com/downloads) (Check: Add to PATH)
- [CMake (Windows Installer)](https://cmake.org/download/) (Check: Add to PATH)
- [Visual Studio 2022 Community Edition](https://visualstudio.microsoft.com/downloads/#visual-studio-community-2022)
  - Select 'Desktop development with C++'
  - On the right, just select:
    - for x64 architecture: `MSVC v143 VS 2022 C++ x64/x86-Buildtools`, `C++ ATL for latest v143 build tools (x86 & x64)`
    - for ARM64 architecture: `MSVC v143 VS 2022 C++ ARM64/ARM64EC-Buildtools`, `C++ ATL for latest v143 build tools (ARM64/ARM64EC)`
    - and latest `Windows 11 SDK`.
  - Everything else is not needed.
- [Win64 OpenSSL](https://slproweb.com/products/Win32OpenSSL.html)
  - [x64 direct link](https://slproweb.com/download/Win64OpenSSL-3_5_0.exe)
  - [ARM64 direct link](https://slproweb.com/download/Win64ARMOpenSSL-3_5_0.exe)
- [Python 3 Windows installer (64-bit or ARM64)](https://www.python.org/downloads/windows/) (Check: Add to PATH and Debug Symbols)
  - Open a console window and execute `pip install aqtinstall`.
  - Now we can download Qt to _C:\Qt_:
    - for x64 architecture: `mkdir c:\Qt && aqt install-qt -O c:\Qt windows desktop 6.8.3 win64_msvc2022_64 -m qtserialport qtwebsockets`
    - for ARM64 architecture: `mkdir c:\Qt && aqt install-qt -O c:\Qt windows_arm64 desktop 6.8.3 win64_msvc2022_arm64 -m qtserialport qtwebsockets`
- [libjpeg-turbo SDK for Visual C++](https://sourceforge.net/projects/libjpeg-turbo/files/)
  - Download the latest 64bit installer (currently `libjpeg-turbo-3.0.1-vc64.exe`) and install to its default location `C:\libjpeg-turbo64`.

###  Optional:
- For package creation:
  - [Inno Setup 6.x](https://jrsoftware.org/isinfo.php) ([direct link](https://jrsoftware.org/download.php/is.exe?site=1))

# Compiling and installing Hyperion

## The general quick way (without big comments)

**complete automated process (Linux only):**
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
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
if this get stucked and dmesg says out of memory try:
cmake --build . -j 2
# optional: install into your system
sudo cmake --build . --target install/strip
# to uninstall (not very well tested, please keep that in mind)
sudo cmake --build . --target uninstall
# ... or run it from compile directory
bin/hyperiond
# webui is located on localhost:8090 or 8091
```

In case you would like to build with a dedicated Qt version, Either supply ``QTDIR`` as ``-DQTDIR=<path>`` to CMake or set an environment variable ``QTDIR`` pointing to the Qt installation.

On Windows MSVC2022 set it via the CMakeSettings.json:
```posh
  "configurations": [
    {
      ...
      "environments": [
        {
          "QTDIR": "C:/Qt/6.5.3/msvc2019_64/"
        }
      ]
    },
```

## The detailed way (with many comments)

### 1. Download:
 Checkout the code from GitHub
```console
git clone --recursive --depth 1 https://github.com/hyperion-project/hyperion.ng.git hyperion
```

### 2. Prepare:
Change into hyperion folder and create a build folder
```console
cd hyperion
mkdir build
cd build
```

### 3. Configure:

> [!IMPORTANT]
> **Windows** developers may need to use the "x64" or "AMR64" native build tools. \
An easy way to do that is to run the shortcut "Native Tools Command
Prompt" for the architecture/version of Visual Studio that you have installed.

To generate the configuration files with automatic platform detection and default settings:
This should fit to **RPI (arm), x64, Windows, macOS**:
```console
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
```

**Developers** should use:
```console
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug ..
```

### 4. Make it:
```console
ninja
```

### 5. Additionals (Linux)

**Install hyperion into your system:** \
Copies all required files to ``/usr/local/share/hyperion``
```console
sudo cmake --build . --target install/strip
```

**If you want to install into another location call this before installing:**
```console
cmake -G Ninja -DCMAKE_INSTALL_PREFIX=/home/pi/apps ..
```
This will install to ``/home/pi/apps/share/hyperion``


