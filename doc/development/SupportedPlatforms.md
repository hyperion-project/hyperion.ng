# Hyperion - Supported platforms
Hyperion is currently suported on the following sets of configuration:
> **_NOTE:_** Configurations tagged as unofficial are running in general, but are provided/maintained on a best effort basis.\
In case of problems, it is recommended checking with the wider Hyperion community (https://hyperion-project.org/forum/).

## Official
| Hardware  | OS              | Version            | Grabber                                 | Package                                                                       | Comments                           |
|-----------|-----------------|--------------------|-----------------------------------------|-------------------------------------------------------------------------------|------------------------------------|
| X64       | Windows         | 10                 | QT&#xB9;                                | [Windows-AMD64.exe](https://github.com/hyperion-project/hyperion.ng/releases) | Direct X9 Grabber via self-compile |
| X64       | Ubuntu          | 18.04, 20.04&#xB2; | QT&#xB9;<br/>XCB/X11&#xB9;              | [Linux-x86_64.deb](https://github.com/hyperion-project/hyperion.ng/releases)  |                                    |
| X64       | Debian          | 9, 10&#xB3;        | QT&#xB9;<br/>XCB/X11&#xB9;              | [Linux-x86_64.deb](https://github.com/hyperion-project/hyperion.ng/releases)  |                                    |
| RPi 4     | HyperBian       | 9, 10&#xB3;        | QT&#xB9;<br/>XCB/X11&#xB9;<br/>DispmanX | [HyperBian.zip](https://github.com/Hyperion-Project/HyperBian/releases)       |                                    |
| RPi 4     | Raspberry Pi OS | 9, 10&#xB3;        | QT&#xB9;<br/>XCB/X11&#xB9;<br/>DispmanX | [Linux-armv7l.deb](https://github.com/hyperion-project/hyperion.ng/releases)  |                                    |
| RPi 3 /3+ | HyperBian       | 9, 10&#xB3;        | QT&#xB9;<br/>XCB/X11&#xB9;<br/>DispmanX | [HyperBian.zip](https://github.com/hyperion-project/hyperion.ng/releases)     |                                    |
| RPi 3 /3+ | Raspberry Pi OS | 9, 10&#xB3;        | QT&#xB9;<br/>XCB/X11&#xB9;<br/>DispmanX | [Linux-armv7l.deb](https://github.com/hyperion-project/hyperion.ng/releases)  |                                    |

## Unofficial
In case you have an additional working setups you would like to share with the community, please get in touch or issue a PR to have the table updated.

| Hardware      | OS              | Version       | Grabber                                 | Package                                                                         | Comments                                                                                                                                                |
|---------------|-----------------|---------------|-----------------------------------------|---------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------|
| X64           | macOS           | 11            | QT<br>OSX                               | [macOS-x86_64.tar.gz](https://github.com/hyperion-project/hyperion.ng/releases) | M1 not tested                                                                                                                                           |
| X64           | Fedora          |               | QT&#xB9;<br/>XCB/X11&#xB9;              | [Linux-x86_64.rpm](https://github.com/hyperion-project/hyperion.ng/releases)    |                                                                                                                                                         |
| X64           | Arch            |               | QT&#xB9;<br/>XCB/X11&#xB9;              | [Linux-x86_64.rpm](https://github.com/hyperion-project/hyperion.ng/releases)    |                                                                                                                                                         |
| RPi 0/ 1 / 2  | Raspberry Pi OS | 9, 10&#xB3;   | QT&#xB9;<br/>XCB/X11&#xB9;<br/>DispmanX | [Linux-armv6l.tar.gz](https://github.com/hyperion-project/hyperion.ng/releases) | No recommended                                                                                                                                          |
| RPi 4         | LibreElec       | 10.x (Matrix) | QT&#xB9;                                | [Linux-armv7l.tar.gz](https://github.com/hyperion-project/hyperion.ng/releases) | [Install on LibreELEC](https://hyperion-project.org/forum/index.php?thread/10463-install-hyperion-ng-on-libreelec-x86-64-rpi-inoffiziell-unofficially/) |
| RPi 4         | LibreElec       | 9.2.x (Leia)  | QT&#xB9;<br/>DispmanX                   | [Linux-armv7l.tar.gz](https://github.com/hyperion-project/hyperion.ng/releases) | [Install on LibreELEC](https://hyperion-project.org/forum/index.php?thread/10463-install-hyperion-ng-on-libreelec-x86-64-rpi-inoffiziell-unofficially/) |
| RPi 3 /3+     | LibreElec       | 10.x (Matrix) | QT&#xB9;                                | [Linux-armv7l.tar.gz](https://github.com/hyperion-project/hyperion.ng/releases) | [Install on LibreELEC](https://hyperion-project.org/forum/index.php?thread/10463-install-hyperion-ng-on-libreelec-x86-64-rpi-inoffiziell-unofficially/) |
| RPi 3 /3+     | LibreElec       | 9.2.x (Leia)  | QT&#xB9;<br/>DispmanX                   | [Linux-armv7l.tar.gz](https://github.com/hyperion-project/hyperion.ng/releases) | [Install on LibreELEC](https://hyperion-project.org/forum/index.php?thread/10463-install-hyperion-ng-on-libreelec-x86-64-rpi-inoffiziell-unofficially/) |
| Amlogic       | CoreElec        | 19.x (Matrix) | Amlogic                                 | CoreElec Plugin                                                                 | Supported via CoreElec project                                                                                                                          |
| Amlogic       | CoreElec        | 9.2.x (Leia)  | Amlogic                                 | CoreElec Plugin                                                                 | Supported via CoreElec project                                                                                                                          |
| Vero4K        | OSMC            |               |                                         |                                                                                 | [hyperion-vero4k](https://github.com/hissingshark/hyperion-vero4k)                                                                                      |
| LG TV         | webOS           |               | -                                       |                                                                                 | [hyperion-webos](https://github.com/webosbrew/hyperion-webos)                                                                                           |

Legend
---
&#xB9; Requires an environment with `DISPLAY` defined\
&#xB2; 18=Bionic Beaver 20=Focal Fossa\
&#xB3; 9=Stretch, 10=Buster

