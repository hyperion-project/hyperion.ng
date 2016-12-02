HYPERION
========

This is a pre alpha development repository for the next major version of hyperion

Feel free to join us! We are looking always for people who wants to participate.





Hyperion is an opensource 'AmbiLight' implementation supported by many devices. The main features of Hyperion are:
* Low CPU load. For a led string of 50 leds the CPU usage will typically be below 1.5% on a non-overclocked Pi.
* Json interface which allows easy integration into scripts.
* A command line utility allows easy testing and configuration of the color transforms (Transformation settings are not preserved over a restart at the moment...).
* Priority channels are not coupled to a specific led data provider which means that a provider can post led data and leave without the need to maintain a connection to Hyperion. This is ideal for a remote application (like our Android app).
* HyperCon. A tool which helps generate a Hyperion configuration file.
* Kodi-checker which checks the playing status of Kodi and decides whether or not to capture the screen.
* Black border detector.
* A scriptable effect engine.
* Generic software architecture to support new devices and new algorithms easily.

More information can be found on the official Hyperion [Wiki](https://wiki.hyperion-project.org) 

If you need further support please open a topic at the our new forum!
[Hyperion webpage/forum](https://www.hyperion-project.org).

## Building

See [Compilehowto](CompileHowto.txt) and [CrossCompileHowto](CrossCompileHowto.txt).

### Debian
Current new deps (libs)
QT5
- sudo apt-get install libqt5core5a libqt5network5 libqt5gui5 libqt5serialport5 libusb-1.0-0

zeroconf
- apt-get install libavahi-core-dev libavahi-compat-libdnssd-dev

94MB free disc space for deps

### Arch
See [AUR](https://aur.archlinux.org/packages/?O=0&SeB=nd&K=hyperion&outdated=&SB=n&SO=a&PP=50&do_Search=Go) for PKGBUILDs on arch. If the PKGBUILD does not work ask questions there please.

## License
The source is released under MIT-License (see http://opensource.org/licenses/MIT).
