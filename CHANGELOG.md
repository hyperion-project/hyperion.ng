# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased](https://github.com/hyperion-project/hyperion.ng/compare/2.0.0-alpha.6...HEAD)

### Breaking

### Added
- [HyperBian](https://github.com/hyperion-project/HyperBian/releases) - A Raspbian Lite image with Hyperion pre installed. (#832)
- An option to reset (delete) the database for the commandline has been added (#820)
- Improve language selection usability (#812)
- readded V4L2 Input method from old Hyperion (#825)
- Windows: Start Hyperion with a console window `hyperiond -c` (Or new start menu entry) (#860)

### Changed
- Updated dependency rpi_ws281x to latest upstream (#820)
- Updated websocket-extensions (#826)
- webui: Suppress default password warning (#830)
- webui: Add French, Vietnamese and Turkish (#842)

### Fixed
- device: Nanoleaf (#829)
- device: LPD8806 Problems fixed (#829)
- Possible crash on shutdown (#846). Issue #668
- Enumerate only V4L2 frame sizes & intervals for framesize type DISCRETE (fix BCM2835 ISP) (#820)
- Fix systemd registration in debian for RPi4 (#820)
- Fix missing define in Profiler & added header notes (#820)
- some Windows Compile issues
- some little things, as always
### Removed

## [2.0.0-alpha.6](https://github.com/hyperion-project/hyperion.ng/releases/tag/2.0.0-alpha.6) - 2020-05-27
### Breaking
The release package names have been adjusted.\
If you used a `.deb` package please uninstall it before you upgrade
- Check for the package name `apt-cache search hyperion`. You may see now a entry like `hyperion-x86`, `hyperion-rpi`
- Remove with the correct name `sudo apt-get remove hyperion-XX`
- Now install the new .deb as usual. From now on the package is just called `hyperion`

### Added
- [Documentation](https://docs.hyperion-project.org) for Hyperion (#780)
- Hyperion can be compiled on windows. [Issues list](https://github.com/hyperion-project/hyperion.ng/issues/737) (#738)
- effect: New plasma/lava lamp effect (#792)
- device: Philips Hue Entertainment API now available as new device. [Read about requirements](https://docs.hyperion-project.org/en/user/LedDevices.html#philipshue) (#806)
- webui: Philips Hue Entertainment API Wizard (#806)
- webui: Toggle-Buttons for Component Remote Control (#677) (#778)
- webui: Add Trapezoid to LED Layout creation (#791)

### Changed
- webui: Dark mode adjustments (#789)

### Fixed
- device: Rewrite-/LatchTime definitions for all devices adjusted (#785) (#799)
- device: Nanoleaf crashes hyperion
- Documentation for Hyperion (#780)
- webui: Hide v4l2 if not available (#782)
- Documentation fixes (#790)

### Removed
- Linux 32bit packages are no longer available for download. Also the `.sh` packages are gone.
- Mac `.dmg` package disabled because of new apple restrictions. Please use the `tar.gz` instead.

## [2.0.0-alpha.5](https://github.com/hyperion-project/hyperion.ng/releases/tag/2.0.0-alpha.5) - 2020-04-17
### Added:
- WebUI Dark Mode (#752) (#765)

### Fixed:
- SSDP Discovery reliability (#756)
- Some effects are running extreme slowly (#763)
- JsonAPI error "Dead lock detected" (2e578c1)
- USB Capture only in Black and white (#766)

### Changed:
- Check if requested Instance is running (#759)
- V4L2 enhancements (#766)
- Stages are now used in Azure CI/CD (9ca197e)

## [2.0.0-alpha.4](https://github.com/hyperion-project/hyperion.ng/releases/tag/2.0.0-alpha.4) - 2020-03-27
### Fixed:
- Memoryleaks & Coredump, if no Grabber compiled (#724)
- Resolve enable state for v4l and screen capture (#728)
- Enable/Disable loops for components
- Runs now on x86_64 LibreElec (missing libs) (#736)
- Brightness componsation is now visible for configuration (#746)
- Prevent malformed image size for effects with specific led layouts (#746)

### Changed:
- SIGUSR1/SIGUSR2 implemented again (#725)
- V4L2 width/height/fps options available (#734)
- Swedish translation update

## [2.0.0-alpha.3](https://github.com/hyperion-project/hyperion.ng/releases/tag/2.0.0-alpha.3) - 2020-03-01
### Added
- Package Update Descriptions on WebUi (#694)
- Pull Requests will now create artifacts for faster testing!

### Fixed
- Led Matrix Layout - Save/Restore (#669) (#697)
- New hue user generation (#696)
- Nanoleaf - Udp Network init was missing (#698)
- Multiple memory leaks and segfault issues in Flatbuffer Forwarder

## [2.0.0-alpha.2](https://github.com/hyperion-project/hyperion.ng/releases/tag/2.0.0-alpha.2) - 2020-02-20
### Added
- Swedish Translation (#675)
- Dutch translation from [Kees](mailto:kees@vannieuwenhuijzen.com), [Nick](mailto:nick@doingcode.nl) & [Ward Wygaerts](mailto:wardwygaerts@gmail.com)
- Polish translation from [Patryk Niedźwiedziński](mailto:pniedzwiedzinski19@gmail.com)
- Romanian translation from [Ghenciu Ciprian](mailto:g.ciprian@osn.ro)

### Changed
- Smoothing comp state on startup (#685)
- Azure GitHub release title (#686)
- SSL/Avahi problems in previous release (#689)
- WebUI Version Check to SemVer. Also addes "Alpha" Channel (#692)

### Removed
- Travis CI tests (#684)

## [2.0.0-alpha.1](https://github.com/hyperion-project/hyperion.ng/releases/tag/2.0.0-alpha.1) - 2020-02-16
### Added
- Initial Release
