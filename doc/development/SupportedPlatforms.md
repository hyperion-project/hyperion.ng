# Hyperion - Supported platforms
The GitHub releases of Hyperion are currently supported on the following sets of configuration:
> [NOTE]
> Configurations tagged as unofficial are running in general, but are provided/maintained on a best effort basis.\
> In case of problems, it is recommended checking with the wider Hyperion community (https://hyperion-project.org/forum/).

## Official
| Hardware  | OS                    | Version                   | Screen-Grabber                          | Package                                                                       | Comments                           |
|-----------|-----------------------|---------------------------|-----------------------------------------|-------------------------------------------------------------------------------|------------------------------------|
| amd64     | Windows               | 11                        | DDA<br/>QT                              | [windows-x64.exe](https://github.com/hyperion-project/hyperion.ng/releases)   | DirectX9 Grabber via self-compile  |
| amd64     | macOS (Intel)         | 13                        | QT<br>OSX                               | [macOS-x86_x64.dmg](https://github.com/hyperion-project/hyperion.ng/releases) |                                    |
| arm64     | macOS (Apple Silicon) | 14, 15                    | QT<br>OSX                               | [macOS-arm64.dmg](https://github.com/hyperion-project/hyperion.ng/releases)   |                                    |
| amd64     | Ubuntu                | 22.04, 24.04, 25.04&#xB2; | QT&#xB9;<br/>XCB/X11&#xB9;              | [Linux-amd64.deb](https://github.com/hyperion-project/hyperion.ng/releases)   |                                    |
| amd64     | Debian                | 10, 11, 12, 13&#xB3;      | QT&#xB9;<br/>XCB/X11&#xB9;              | [Linux-amd.deb](https://github.com/hyperion-project/hyperion.ng/releases)     |                                    |
| RPi 5     | HyperBian             | 10, 11, 12, 13&#xB3;      | QT&#xB9;<br/>XCB/X11&#xB9;<br/>DispmanX | [HyperBian.zip](https://github.com/Hyperion-Project/HyperBian/releases)       |                                    |
| RPi 5     | Raspberry Pi OS       | 12, 13&#xB3;              | QT&#xB9;<br/>XCB/X11&#xB9;<br/>DispmanX | [Linux-arm64.deb](https://github.com/hyperion-project/hyperion.ng/releases)   |                                    |
| RPi 4     | HyperBian             | 10, 11, 12, 13&#xB3;      | QT&#xB9;<br/>XCB/X11&#xB9;<br/>DispmanX | [HyperBian.zip](https://github.com/Hyperion-Project/HyperBian/releases)       |                                    |
| RPi 4     | Raspberry Pi OS       | 10, 11, 12, 13&#xB3;      | QT&#xB9;<br/>XCB/X11&#xB9;<br/>DispmanX | [Linux-armv7l.deb](https://github.com/hyperion-project/hyperion.ng/releases)  |                                    |
| RPi 3 /3+ | HyperBian             | 10, 11, 12, 13&#xB3;      | QT&#xB9;<br/>XCB/X11&#xB9;<br/>DispmanX | [HyperBian.zip](https://github.com/hyperion-project/hyperion.ng/releases)     |                                    |
| RPi 3 /3+ | Raspberry Pi OS       | 10, 11, 12, 13&#xB3;      | QT&#xB9;<br/>XCB/X11&#xB9;<br/>DispmanX | [Linux-armv7l.deb](https://github.com/hyperion-project/hyperion.ng/releases)  |                                    |

## Unofficial
In case you have an additional working setups you would like to share with the community, please get in touch or issue a PR to have the table updated.

| Hardware      | OS              | Version         | Screen-Grabber                          | Package                                                                         | Comments                                                                                                                                                |
|---------------|-----------------|-----------------|-----------------------------------------|---------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------|
| arm64         | Windows         | 11              | DDA<br/>QT                              | [windows-arm64.exe](https://github.com/hyperion-project/hyperion.ng/releases)
| amd64         | Fedora          | 42              | QT&#xB9;<br/>XCB/X11&#xB9;              | [Linux-x86_64.rpm](https://github.com/hyperion-project/hyperion.ng/releases)    |                                                                                                                                                         |
| amd64         | Arch            |                 | QT&#xB9;<br/>XCB/X11&#xB9;              | [Linux-x86_64.rpm](https://github.com/hyperion-project/hyperion.ng/releases)    |                                                                                                                                                         |
| RPi 0/ 1 / 2  | Raspberry Pi OS | 10, 11, 12&#xB3;| QT&#xB9;<br/>XCB/X11&#xB9;<br/>DispmanX | [Linux-armv6l.tar.gz](https://github.com/hyperion-project/hyperion.ng/releases) | No recommended                                                                                                                                          |
| X64           | LibreElec       | 11.x (Nexus)    | [Kodi add-on](https://github.com/hyperion-project/hyperion.kodi/releases) | [Linux-x86_64.tar.gz](https://github.com/hyperion-project/hyperion.ng/releases) | [Install on LibreELEC](https://hyperion-project.org/forum/index.php?thread/13754-install-update-hyperion-ng-on-libreelec/) |
| RPi 4         | LibreElec       | 11.x (Nexus)    | -                                       | [Linux-armv7l.tar.gz](https://github.com/hyperion-project/hyperion.ng/releases) | [Install on LibreELEC](https://hyperion-project.org/forum/index.php?thread/13754-install-update-hyperion-ng-on-libreelec/) |
| RPi 4         | LibreElec       | 10.x (Matrix)   | -                                       | [Linux-armv7l.tar.gz](https://github.com/hyperion-project/hyperion.ng/releases) | [Install on LibreELEC](https://hyperion-project.org/forum/index.php?thread/13754-install-update-hyperion-ng-on-libreelec/) |
| RPi 4         | LibreElec       | 9.2.x (Leia)    | QT&#xB9;<br/>DispmanX                   | [Linux-armv7l.tar.gz](https://github.com/hyperion-project/hyperion.ng/releases) | [Install on LibreELEC](https://hyperion-project.org/forum/index.php?thread/13754-install-update-hyperion-ng-on-libreelec/) |
| RPi 3 /3+     | LibreElec       | 11.x (Nexus)    | -                                       | [Linux-armv7l.tar.gz](https://github.com/hyperion-project/hyperion.ng/releases) | [Install on LibreELEC](https://hyperion-project.org/forum/index.php?thread/13754-install-update-hyperion-ng-on-libreelec/) |
| RPi 3 /3+     | LibreElec       | 10.x (Matrix)   | -                                       | [Linux-armv7l.tar.gz](https://github.com/hyperion-project/hyperion.ng/releases) | [Install on LibreELEC](https://hyperion-project.org/forum/index.php?thread/13754-install-update-hyperion-ng-on-libreelec/) |
| RPi 3 /3+     | LibreElec       | 9.2.x (Leia)    | QT&#xB9;<br/>DispmanX                   | [Linux-armv7l.tar.gz](https://github.com/hyperion-project/hyperion.ng/releases) | [Install on LibreELEC](https://hyperion-project.org/forum/index.php?thread/13754-install-update-hyperion-ng-on-libreelec/) |
| Amlogic       | CoreElec        | 21.x (Omega)    | Amlogic                                 | CoreElec Plugin                                                                 | Supported via CoreElec project                                                                                                                          |
| Amlogic       | CoreElec        | 20.x (Nexus)    | Amlogic                                 | CoreElec Plugin                                                                 | Supported via CoreElec project                                                                                                                          |
| Amlogic       | CoreElec        | 19.x (Matrix)   | Amlogic                                 | CoreElec Plugin                                                                 | Supported via CoreElec project                                                                                                                          |
| Amlogic       | CoreElec        | 9.2.x (Leia)    | Amlogic                                 | CoreElec Plugin                                                                 | Supported via CoreElec project                                                                                                                          |
| Vero4K        | OSMC            |                 |                                         |                                                                                 | [hyperion-vero4k](https://github.com/hissingshark/hyperion-vero4k)                                                                                      |
| LG TV         | webOS           |                 | -                                       |                                                                                 | [hyperion-webos](https://github.com/webosbrew/hyperion-webos)                                                                                           |

Legend
---
&#xB9; Requires an environment with `DISPLAY` defined\
&#xB2; 22=Jammy Jellyfish, 24=Noble Numbat, 25=Plucky Puffin\
&#xB3; 10=Buster, 11=Bullseye, 12=Bookworm, 13=Trixie
