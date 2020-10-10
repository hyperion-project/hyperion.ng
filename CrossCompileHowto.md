# Cross-Compile Hyperion-NG
Leverage the power of a host environment (here Ubuntu) compiling for a target platform (here Raspberry Pi).
Use a clean Raspbian Stretch Lite (on target) and Ubuntu 18/19 (on host) to execute the steps outlined below.
## On the Target system (here Raspberry Pi)
Install required additional packages.
```
sudo apt-get install qtbase5-dev libqt5serialport5-dev libqt5svg5-dev libusb-1.0-0-dev python3-dev libcec-dev libxcb-util0-dev libxcb-randr0-dev libxrandr-dev libxrender-dev libavahi-core-dev libavahi-compat-libdnssd-dev libjpeg-dev libturbojpeg0-dev libqt5sql5-sqlite aptitude qt5-default rsync libssl-dev zlib1g-dev
```
## On the Host system (here Ubuntu)
Update the Ubuntu environment to the latest stage and install required additional packages.
```
sudo apt-get update
sudo apt-get upgrade
sudo apt-get -qq -y install git rsync cmake build-essential qtbase5-dev libqt5serialport5-dev libqt5svg5-dev libqt5sql5-sqlite libqt5x11extras5-dev libusb-1.0-0-dev python3-dev libcec-dev libxcb-image0-dev libxcb-util0-dev libxcb-shm0-dev libxcb-render0-dev libxcb-randr0-dev libxrandr-dev libxrender-dev libavahi-core-dev libavahi-compat-libdnssd-dev libjpeg-dev libturbojpeg0-dev libssl-dev zlib1g-dev
```

Refine the target IP or hostname, plus userID as required and set-up cross-compilation environment:
```
export TARGET_IP=x.x.x.x
export TARGET_USER=pi
```
```
export CROSSROOT="$HOME/crosscompile"
export RASCROSS_DIR="$CROSSROOT/raspberrypi"
export ROOTFS_DIR="$RASCROSS_DIR/rootfs"
export TOOLCHAIN_DIR="$RASCROSS_DIR/tools"
export QT5_DIR="$CROSSROOT/Qt5"
export HYPERION_DIR="$HOME/hyperion.ng"
```
Get native files from target platform into the host-environment:
```
mkdir -p "$ROOTFS_DIR/lib"
mkdir -p "$ROOTFS_DIR/usr"
rsync -rl --delete-after --copy-unsafe-links $TARGET_USER@$TARGET_IP:/lib "$ROOTFS_DIR"
rsync -rl --delete-after --copy-unsafe-links $TARGET_USER@$TARGET_IP:/usr/include "$ROOTFS_DIR/usr"
rsync -rl --delete-after --copy-unsafe-links $TARGET_USER@$TARGET_IP:/usr/lib "$ROOTFS_DIR/usr"
```

### Raspberry Pi specific steps
Get Raspberry Pi firmware:
```
mkdir -p "$RASCROSS_DIR/firmware"
git clone --depth 1 https://github.com/raspberrypi/firmware.git "$RASCROSS_DIR/firmware"
ln -s "$RASCROSS_DIR/firmware/hardfp/opt" "$ROOTFS_DIR/opt"
```
Get toolchain files which allows to build ARM executables on x86 platforms:
```
mkdir -p "$TOOLCHAIN_DIR"
cd $TOOLCHAIN_DIR
wget -c https://releases.linaro.org/components/toolchain/binaries/7.4-2019.02/arm-linux-gnueabihf/gcc-linaro-7.4.1-2019.02-x86_64_arm-linux-gnueabihf.tar.xz --no-check-certificate
tar -xvf gcc-linaro-7.4.1-2019.02-x86_64_arm-linux-gnueabihf.tar.xz

```
### Install the Qt5 framework
```
mkdir -p "$QT5_DIR"
cd "$QT5_DIR"
wget -c https://download.qt.io/new_archive/qt/5.7/5.7.1/qt-opensource-linux-x64-5.7.1.run
chmod +x $QT5_DIR/*.run
```
Display absolute installation directory to be used in Qt5 installer:
```
echo $HOME/crosscompile/Qt5
```
Start the Qt5 installation.
Follow the dialogs and install in absolute directory of ```$HOME/crosscompile/Qt5``` (copy from above)
```
./qt-opensource-linux-x64-5.7.1.run
```
### Get the Hyperion-NG source files
```
git clone --recursive https://github.com/hyperion-project/hyperion.ng.git "$HYPERION_DIR"
```
### Get required submodules for Hyperion
```
cd "$HYPERION_DIR"
git fetch --recurse-submodules -j2
```
### Compile Hyperion-NG
```
cd "$HYPERION_DIR"
chmod +x "$HYPERION_DIR/bin/"*.sh
./bin/create_all_releases.sh
```
### Transfer output packages to target platform and install Hyperion-NG
Output packages for target platform (.deb, .tar.gz, .sh) can be found here:
```
$HYPERION_DIR/deploy/rpi
```
# Install Hyperion-NG on target platform
t.b.d.


