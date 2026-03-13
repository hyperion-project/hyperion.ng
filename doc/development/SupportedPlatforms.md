# Hyperion - Supported platforms
Hyperion is currently supported on the following configurations via our package repository or via GitHub releases.

> [NOTE]
> Configurations tagged as unofficial are running in general, but are provided/maintained on a best effort basis.\
> In case of problems, it is recommended checking with the wider [Hyperion community forum](https://hyperion-project.org/forum/).

## Official
| Hardware     | OS                    | Version                          | Screen-Grabber                                        | Installation                                                                                      | Comments                          |
| ------------ | --------------------- | -------------------------------- | ----------------------------------------------------- | ------------------------------------------------------------------------------------------------- | --------------------------------- |
| amd64        | Windows               | 11                               | DDA<br/>QT                                            | [Windows-x64.exe](https://github.com/hyperion-project/hyperion.ng/releases)                       | DirectX9 Grabber via self-compile |
| arm64        | Windows               | 11                               | DDA<br/>QT                                            | [Windows-arm64.exe](https://github.com/hyperion-project/hyperion.ng/releases)                     |                                   |
| amd64        | macOS (Intel)         | 13                               | QT<br>OSX                                             | [macOS-x86_64.dmg ](https://github.com/hyperion-project/hyperion.ng/releases)                     |                                   |
| arm64        | macOS (Apple Silicon) | 14, 15                           | QT<br>OSX                                             | [macOS-arm64.dmg](https://github.com/hyperion-project/hyperion.ng/releases)                       |                                   |
| amd64        | Ubuntu (based)        | 22.04, 24.04, 25.10, 26.04&#xB3; | QT&#xB9;<br/>XCB/X11&#xB9;                            | [via Easy Install](https://docs.hyperion-project.org/user/gettingstarted/Linux.html#easy-install) |                                   |
| amd64        | Debian (based)        | 11, 12, 13&#xB3;                 | QT&#xB9;<br/>XCB/X11&#xB9;                            | [via Easy Install](https://docs.hyperion-project.org/user/gettingstarted/Linux.html#easy-install) |                                   |
| amd64        | Fedora                | 43                               | QT&#xB9;<br/>XCB/X11&#xB9;                            | [via Easy Install](https://docs.hyperion-project.org/user/gettingstarted/Linux.html#easy-install) |                                   |
| RPi 5        | HyperBian             | 13&#xB3;                         | -                                                     | [HyperBian.zip](https://github.com/Hyperion-Project/HyperBian/releases)                           | Hyperion Out-of-a-box             |
| RPi 4        | HyperBian             | 13&#xB3;                         | -                                                     | [HyperBian.zip](https://github.com/Hyperion-Project/HyperBian/releases)                           | Hyperion Out-of-a-box             |
| RPi 3 /3+    | HyperBian             | 13&#xB3;                         | -                                                     | [HyperBian.zip](https://github.com/hyperion-project/hyperion.ng/releases)                         | Hyperion Out-of-a-box             |
| RPi 5        | Raspberry Pi OS       | 12, 13&#xB3;                     | DRM&#xB2;<br/>QT&#xB9;<br/>XCB/X11&#xB9;<br/>DispmanX | [via Easy Install](https://docs.hyperion-project.org/user/gettingstarted/Linux.html#easy-install) |                                   |
| RPi 4        | Raspberry Pi OS       | 11, 12, 13&#xB3;                 | DRM&#xB2;<br/>QT&#xB9;<br/>XCB/X11&#xB9;<br/>DispmanX | [via Easy Install](https://docs.hyperion-project.org/user/gettingstarted/Linux.html#easy-install) |                                   |
| RPi 3 /3+    | Raspberry Pi OS       | 11, 12, 13&#xB3;                 | QT&#xB9;<br/>XCB/X11&#xB9;<br/>DispmanX               | [via Easy Install](https://docs.hyperion-project.org/user/gettingstarted/Linux.html#easy-install) |                                   |
| RPi 0/ 1 / 2 | Raspberry Pi OS       | 11, 12&#xB3;                     | QT&#xB9;<br/>XCB/X11&#xB9;<br/>DispmanX               | [via Easy Install](https://docs.hyperion-project.org/user/gettingstarted/Linux.html#easy-install) |                                   |

## Unofficial
In case you have an additional working setups you would like to share with the community, please get in touch or issue a PR to have the table updated.

| Hardware  | OS        | Version      | Screen-Grabber                                                            | Installation                                                                                      | Comments                                                           |
| --------- | --------- | ------------ | ------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------ |
| X64       | LibreElec | 12.x (Omega) | [Kodi add-on](https://github.com/hyperion-project/hyperion.kodi/releases) | [via Easy Install](https://docs.hyperion-project.org/user/gettingstarted/Linux.html#easy-install) |                                                                    |
| RPi 5     | LibreElec | 12.x (Omega) | -                                                                         | [via Easy Install](https://docs.hyperion-project.org/user/gettingstarted/Linux.html#easy-install) |                                                                    |
| RPi 4     | LibreElec | 12.x (Omega) | -                                                                         | [via Easy Install](https://docs.hyperion-project.org/user/gettingstarted/Linux.html#easy-install) |                                                                    |
| RPi 3 /3+ | LibreElec | 12.x (Omega) | -                                                                         | [via Easy Install](https://docs.hyperion-project.org/user/gettingstarted/Linux.html#easy-install) |                                                                    |
| RPi 4     | LibreElec | 9.2.x (Leia) | QT&#xB9;<br/>DispmanX                                                     | [via Easy Install](https://docs.hyperion-project.org/user/gettingstarted/Linux.html#easy-install) |                                                                    |
| RPi 3 /3+ | LibreElec | 9.2.x (Leia) | QT&#xB9;<br/>DispmanX                                                     | [via Easy Install](https://docs.hyperion-project.org/user/gettingstarted/Linux.html#easy-install) |                                                                    |
| Amlogic   | CoreElec  | 22.x (Piers) | Amlogic                                                                   | CoreElec Plugin                                                                                   | Supported via CoreElec project                                     |
| Amlogic   | CoreElec  | 21.x (Omega) | Amlogic                                                                   | CoreElec Plugin                                                                                   | Supported via CoreElec project                                     |
| Vero4K    | OSMC      |              |                                                                           |                                                                                                   | [hyperion-vero4k](https://github.com/hissingshark/hyperion-vero4k) |
| LG TV     | webOS     |              | -                                                                         |                                                                                                   | [hyperion-webos](https://github.com/webosbrew/hyperion-webos)      |

Legend
---
&#xB9; Requires an X11 based display manager and an environment with `DISPLAY` defined. Wayland is currently not supported\
&#xB2; Requires Hyperion running under root\
&#xB3; Debian: 11=Bullseye, 12=Bookworm, 13=Trixie, Ubuntu: 22=Jammy Jellyfish, 24=Noble Numbat, 25=Questing Quokka, 26=Resolute Raccoon
