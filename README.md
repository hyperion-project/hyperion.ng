HYPERION
========

This is a pre alpha development repository for the next major version of hyperion

--------
## **Important notice!**

Hyperion.NG is under heavy development. This version is currently _only for development_ purpose.
Please do not use it for your 'productiv' setup!

If you want to use hyperion as 'normal user', please use [current stable version](https://github.com/hyperion-project/hyperion)

Besides of that ....  Feel free to join us! We are looking always for people who wants to participate.

--------

Hyperion is an opensource 'AmbiLight' implementation with support for many LED devices and video grabbers.

The main features of Hyperion are:
* Low CPU load makes it perfect for SoCs like Raspberry Pi
* Json interface which allows easy integration into scripts
* A command line utility to for testing and integration in automated environment
* Priority channels are not coupled to a specific led data provider which means that a provider can post led data and leave without the need to maintain a connection to Hyperion. This is ideal for a remote application (like our Android app).
* Black border detector.
* A scriptable (Python) effect engine
* A web ui to configure and remote control hyperion

More information can be found on the official Hyperion [Wiki](https://wiki.hyperion-project.org)

If you need further support please open a topic at the our new forum!
[Hyperion webpage/forum](https://www.hyperion-project.org).

## Requirements
* Debian 9, Ubuntu 16.04 or higher. Windows is not supported currently.

## Building
See [Compilehowto](CompileHowto.md) and [CrossCompileHowto](CrossCompileHowto.txt).

## Download
A download isn't available, you need to compile your own version see "Building"

## License
The source is released under MIT-License (see http://opensource.org/licenses/MIT).
