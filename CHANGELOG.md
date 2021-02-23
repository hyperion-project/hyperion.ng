# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased](https://github.com/hyperion-project/hyperion.ng/compare/2.0.0-alpha.9...HEAD)

### Breaking

### Added

### Changed
- Updated dependency rpi_ws281x to latest upstream
- Fix High CPU load (RPI3B+) (#1013)

- Documentation: Add link to [Hyperion-py](https://github.com/dermotduffy/hyperion-py)

### Fixed

- Fix issue #1127: LED-Devices: Correct total packet count in tpm2net implementation
- LED-Hue: Proper black in Entertainement mode if min brightness is set

### Removed

## [2.0.0-alpha.9](https://github.com/hyperion-project/hyperion.ng/releases/tag/2.0.0-alpha.9) - 2020-11-18
### Added
- Grabber: DirectX9 support (#1039)
- New blackbar detection mode "Letterbox", that considers only bars at the top and bottom of picture

- LED-Devices: Cololight support (Cololight Plus & Strip) incl. configuration wizard
- LED-Devices: SK9822 support (#1005,#1017)

- UX: New language support: Russian and Chinese (simplified) (#1005)
- UX: Additional details on Hardware/CPU information (#1045)
- UX: Systray icons added - Issue #925 (#1040)

- Read-Only configuration database support
- Hide Window Systray icon on Hyperion exit & Install DirectX Redistributable
- Read-Only configuration database support

### Changed
- boblight: reduce cpu time spent on memcopy and parsing rgb values (#1016)
- Windows Installer/Uninstaller notification when Hyperion is running (#1033)
- Updated Windows Dependencies
- Documentation: Optimized images (#1058)
- UX: Default LED-layout is now one LED only to avoid errors as in #673
- UX: Change links from http to https (#1067)
- Change links from http to https (#1067)
- Cleanup packages.cmake & extend NSIS plugin directory
- Optimize images (#1058)
- Docs: Refreshed EN JSON API documentation

### Fixed
- Color calibration for Kodi 18 (#1044)
- LED-Devices: Karatelight, allow an 8-LED configuration (#1037)
- LED-Devices: Save Hue light state between sessions (#1014)
- LED-Devices: LED's retain last state after clearing a source (#1008)
- LED-Devices: Lightpack issue #1015 (#1049)
- Fix various JSON API issues (#1036)
- Fix issue #909, Have ratio correction first and then scale (#1047)
- Fix display argument in hyperion-qt (#1027)
- Fix Python reset thread state
- AVAHI included in Webserver (#996)
- Fix add libcec to deb/rpm dependency list
- Fix Hyperion configuration is corrected during start-up, if required
- Fix color comparison / Signal detection (#1087)

### Removed
- Replace Multi-Lightpack by multi-instance Lightpack configuration (#1049)

## [2.0.0-alpha.8](https://github.com/hyperion-project/hyperion.ng/releases/tag/2.0.0-alpha.8) - 2020-09-14
### Added
- Add XCB grabber, a faster and safer alternative for X11 grabbing (#912)
- for Windows: Add binary meta (#932)
- Differentiate between LED-Device Enable/Disable and Switch On/Off (#960) (Fixes: #828)
- AtmoOrb discovery and identification support (#988)
- New AtmoOrb Wizard (#988)
- Added and updated some language files (#900, #926, #916) (DE, CS, NL, FR, IT, PL, RO, ES, SV, TR, VI)
### Changed
- Improved UDP-Device Error handling (#961)
- NSIS/Systray option to launch Hyperion on Windows start (HKCU) (#887)
- Updated some dependencies (#929, #1003, #1004)
- refactor: Modernize Qt connections (#914)
- refactor: Resolve some clang warnings (#915)
- refactor: Several random fixes + Experimental playground (#917)
- Use query interface for void returning X requests (#945)
- Move Python related code to Python module (#946)
- General tidy up (#958)
- AtmoOrb ESP8266 sketch to support device identification, plus small fix (#988)

### Fixed
- webui: Works now with HTTPS port 443 (#923 with #924)
- Adalight issue (#903 with #991)
- Fixed CI: Trigger HyperBian build after release
- Fixed: -DUSE_SYSTEM_MBEDTLS_LIBS=ON - undefined reference (#898)
- set zlib back to system ignore list/revert pr #871 (#904)
- Fixed: logger and led colors (#906)
- Fixed: some more threading errors (#911)
- Fix OSX build (#952)
- AtmoOrb Fix (#988)
- Return TAN to API requests whenever possible (#1002)

### Removed

## [2.0.0-alpha.7](https://github.com/hyperion-project/hyperion.ng/releases/tag/2.0.0-alpha.7) - 2020-07-23
### Added
- [HyperBian](https://github.com/hyperion-project/HyperBian/releases) - A Raspberry Pi OS Lite image with Hyperion pre installed. (#832)
- An option to reset (delete) the database for the commandline has been added (#820)
- Improve language selection usability (#812)
- re-added V4L2 Input method from old Hyperion (#825)
- Windows: Start Hyperion with a console window `hyperiond -c` (Or new start menu entry) (#860)
- Get process IDs by iterating /proc (#843)
- Dump stack trace on crash (Implement #849) (#870)
- Minor fixes
- New Devices (#875)
  * Yeelight support incl. device discovery and setup-wizard
  * WLED as own device and pre-configuration
- Additional device related capabilities (#875)
  * discover, getProperties, identify, store/restore state and power-on/off available for Philips-Hue, Nanoleaf, Yeelight, partially for Rs232 / USB (Hid)
  * New device capabilities are accessible via JSON-API
  * New REST-API wrapper class in support of network devices, e.g. Philips Hue, Nanoleaf and WLED
  * Flexible SSDP-Discovery incl. RegEx matching and filtering
- Documentation (#875)
  * Process workflow for LED-Devices
  * Documentation of device classes & methods
  * Code template for new LED-Devices available
- CEC detection (#877)

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
- LED-Device workflow changed allowing proper suspend/resume & disable/enable scenarios (#875)
- Network LED-Devices will stop sending packages when disabled (#875)
- Rs232 Provider fully reworked and changed to synchronous writes (#875)
- Rs232 configuration via portname and system location (/dev/ style), auto detection is not case-sensitive any longer (#875)
- Additional error handling depending on device type (#875)
- Add Windows compatibility incl. moving to Qt functions (#875)
- Add compatibility for different Qt versions (#875)


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
- some code improvements & cleanup (#861) (#872) (#880) (#876)
- some little things, as always (#863)
- AtmoOrb: Buffer length fix and new configuration validations (#875)
- Added missing DMX SubTypes to configuration (#875)
- Fix logger (#885)
  * Make logger thread safe
  * Include timestamp in logs
  * Make logs look a bit more cleaner
- Decrease compile time (#886)
- Fix some data synchronization error (#890)
- Fix Qt screenshot crash (#889)
- Fix crash on startup if no X server available (#892)
- Fix RPC restart of Hyperion (#894)

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
- Brightness compensation is now visible for configuration (#746)
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
- WebUI Version Check to SemVer. Also adds "Alpha" Channel (#692)

### Removed
- Travis CI tests (#684)

## [2.0.0-alpha.1](https://github.com/hyperion-project/hyperion.ng/releases/tag/2.0.0-alpha.1) - 2020-02-16
### Added
- Initial Release
