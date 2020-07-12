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
- Get process IDs by iterating /proc (#843)
- Dump stack trace on crash (Implement #849) (#870)
- Minor fixes

### Changed
- Updated dependency rpi_ws281x to latest upstream (#820)
- Updated websocket-extensions (#826)
- webui: Suppress default password warning (#830)
- webui: Add French, Vietnamese and Turkish (#842)
- Show thread names in GDB for better debugging (#848)
- CompileHowto.md updated (#864)
- Updated Embedded python package (zip) for Linux (#871)
- DBManager: ORDER BY parameter added to getRecord(s) (#770)
- Corrected GitHub Actions badge
- Fix GitHub Actions/Azure Windows Workflow/Pipeline
- Updated submodules flatbuffers/rpi_ws281x (#873)
- LED Device Features, Fixes and Refactoring (Resubmit PR855) (#875) (THIS ONE IS HUGE! THX TO @Lord-Grey)
  * Refactor LedDevices - Initial version
  * Small renamings
  * Add WLED as own device
  * Lpd8806 Remove open() method
  * remove dependency on Qt 5.10
  * Lpd8806 Remove open() method
  * Update WS281x
  * Update WS2812SPI
  * Add writeBlack for WLED powerOff
  * WLED remove extra bracket
  * Allow different Nanoleaf panel numbering sequence (Feature req.#827)
  * build(deps): bump websocket-extensions from 0.1.3 to 0.1.4 in /docs (#826)
  * Bumps [websocket-extensions](https://github.com/faye/websocket-extensions-node) from 0.1.3 to 0.1.4.
  - [Release notes](https://github.com/faye/websocket-extensions-node/releases)
  - [Changelog](https://github.com/faye/websocket-extensions-node/blob/master/CHANGELOG.md)
  - [Commits](faye/websocket-extensions-node@0.1.3...0.1.4)
  * Fix typos
  * Nanoleaf clean-up
  * Yeelight support, generalize wizard elements
  * Update Yeelight to handle quota in music mode
  * Yeelight extend rage for extraTimeDarkness for testing
  * Clean-up - Add commentary, Remove development debug statements
  * Fix brightnessSwitchOffOnMinimum typo and default value
  * Yeelight support restoreOriginalState, additional Fixes
  * WLED - Remove UDP-Port, as it is not configurable
  * Fix merging issue
  * Remove QHostAddress::operator=(const QString&)' is deprecated
  * Windows compile errors and (Qt 5.15 deprecation) warnings
  * Fix order includes
  * LedDeviceFile Support Qt5.7 and greater
  * Windows compatibility and other Fixes
  * Fix Qt Version compatability
  * Rs232 - Resolve portname from unix /dev/ style, fix DMX sub-type support
  * Disable WLED Wizard Button (until Wizard is available)
  * Yeelight updates
  * Add wrong log-type as per #505
  * Fixes and Clean-up after clang-tidy report
  * Fix udpe131 not enabled for generated CID
  * Change timer into dynamic for Qt Thread-Affinity
  * Hue clean-up and diyHue workaround
  * Updates after review feedback by m-seker
  * Add "chrono" includes

### Fixed
- device: Nanoleaf (#829)
- device: LPD8806 Problems fixed (#829)
- Possible crash on shutdown (#846). Issue #668
- Enumerate only V4L2 frame sizes & intervals for framesize type DISCRETE (fix BCM2835 ISP) (#820)
- Fix systemd registration in debian for RPi4 (#820)
- Fix missing define in Profiler & added header notes (#820)
- some Windows Compile issues
- Fix: leaking active effects during quit (#850)
- Correct path for each build configuration
- Fix heap corruption (#862)
- Fix OpenSSL dependencies for Windows (#864)
- Fix resolution change event Fixes part of #620 (#867)
- some code improvements & cleanup (#861) (#872)
- some little things, as always (#863)

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
