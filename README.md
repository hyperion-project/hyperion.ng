![Hyperion.NG](https://raw.githubusercontent.com/hyperion-project/hyperion.ng/master/assets/webconfig/img/hyperion/hyperionlogo.png)

[![Dependencies](https://img.shields.io/librariesio/github/hyperion-project/hyperion.ng.svg)](https://github.com/hyperion-project/hyperion.ng/tree/master/dependencies/external)
[![Azure-Pipeline](https://dev.azure.com/Hyperion-Project/Hyperion.NG/_apis/build/status/Hyperion.NG?branchName=master)](https://dev.azure.com/Hyperion-Project/Hyperion.NG/_build/latest?definitionId=7&branchName=master)
[![Travis-CI](https://travis-ci.org/hyperion-project/hyperion.ng.svg?branch=master)](https://travis-ci.org/hyperion-project/hyperion.ng)
[![GitHub Actions](https://github.com/hyperion-project/hyperion.ng/workflows/GitHub%20Actions/badge.svg)](https://github.com/hyperion-project/hyperion.ng/actions)
[![LGTM](https://img.shields.io/lgtm/alerts/g/hyperion-project/hyperion.ng.svg)](https://lgtm.com/projects/g/hyperion-project/hyperion.ng/alerts/)
[![Documentation](https://codedocs.xyz/hyperion-project/hyperion.ng.svg)](https://codedocs.xyz/hyperion-project/hyperion.ng/)

## About Hyperion

[Hyperion.NG](https://github.com/hyperion-project/hyperion.ng) is an opensource [Bias or Ambient Lighting](https://en.wikipedia.org/wiki/Bias_lighting) implementation which you might know from TV manufactures. It supports many LED devices and video grabbers. The project is still in a beta development stage (no stable release available).

![Screenshot](doc/screenshot.png)

### Features:

* Low CPU load makes it perfect for SoCs like Raspberry Pi
* Json interface which allows easy integration into scripts
* A command line utility for testing and integration in automated environment
* Priority channels are not coupled to a specific led data provider which means that a provider can post led data and leave without the need to maintain a connection to Hyperion. This is ideal for a remote application (like our [Android app](https://play.google.com/store/apps/details?id=nl.hyperion.hyperionpro)).
* Black border detector and processor
* A scriptable (Python) effect engine
* A multi language web interface to configure and remote control hyperion

If you need further support please open a topic at the forum!  
[![Hyperion webpage/forum](https://img.shields.io/website/https/hyperion-project.org.svg?down_color=red&down_message=offline&up_color=green&up_message=online)](https://www.hyperion-project.org)

## Contributing  

Contributions are welcome! Feel free to join us! We are looking always for people who wants to participate.  
[![Contributors](https://img.shields.io/github/contributors/hyperion-project/hyperion.ng.svg)](https://github.com/hyperion-project/hyperion.ng/graphs/contributors)

For an example, you can participate in the translation.  
[![Join Translation](https://img.shields.io/badge/POEditor-translate-green.svg)](https://poeditor.com/join/project/Y4F6vHRFjA)

## Requirements
Debian 9, Ubuntu 16.04 or higher. Windows is not supported currently.

We provide a macOS Build but we can not support this.

## Building
See [CompileHowto](CompileHowto.md) and [CrossCompileHowto](CrossCompileHowto.txt).

## Download
**Please be patient. The first release is coming soon.**

## License
The source is released under MIT-License (see http://opensource.org/licenses/MIT).  
[![GitHub license](https://img.shields.io/badge/License-MIT-yellow.svg)](https://raw.githubusercontent.com/hyperion-project/hyperion.ng/master/LICENSE)

