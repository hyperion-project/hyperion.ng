HYPERION
========

Hyperion is an opensource 'AmbiLight' implementation controlled using the RaspBerry Pi running [Raspbmc](http://www.raspbmc.com). The main features of Hyperion are:
* Low CPU load. For a led string of 50 leds the CPU usage will typically be below 1.5% on a non-overclocked Pi.
* Json interface which allows easy integration into scripts.
* A command line utility allows easy testing and configuration of the color transforms (Transformation settings are not preserved over a restart at the moment...).
* Priority channels are not coupled to a specific led data provider which means that a provider can post led data and leave without the need to maintain a connection to Hyperion. This is ideal for a remote application (like our Android app).
* HyperCon. A tool which helps generate a Hyperion configuration file.
* XBMC-checker which checks the playing status of XBMC and decides whether or not to capture the screen.
* Black border detector.
* A scriptable effect engine.
* Generic software architecture to support new devices and new algorithms easily.

More information can be found on the [wiki](https://github.com/tvdzwan/hyperion/wiki) or the [Hyperion topic](http://forum.stmlabs.com/showthread.php?tid=11053) on the Raspbmc forum.

The source is released under MIT-License (see http://opensource.org/licenses/MIT).
