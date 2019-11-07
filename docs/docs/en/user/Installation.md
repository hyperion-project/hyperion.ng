# Install Hyperion
Hyperion supports various platforms for installation, as package or as a portable .zip.

## Requirements

### Supported Hardware/Software
  * Raspberry Pi (See also [HyperBian](/en/user/HyperBian))
  * Debian 9 | Ubuntu 16.04 or higher
  * Mac OS
  * OpenELEC, LibreELEC

**Please note that some arm devices have limited support in terms of direct capturing**

### Supported Browsers
Hyperion will be configured and controlled trough a web configuration. Also for mobile browsers.
  * Chrome 47+
  * Firefox 43+
  * Opera 34+
  * Safari 9.1+
  * Microsoft Edge 14+

::: warning Internet Explorer
Intenet Explorer is not supported
:::

## Install Hyperion
  * Raspberry Pi you can use [HyperBian](/en/user/HyperBian.md) for a fresh start. Or use the install system
  * We provide installation packages (.deb) to install Hyperion with a single click on Debian/Ubuntu based systems.
  * A script installation for read-only systems like OpenELEC, LibreELEC.
  * Mac OSX - currently just a zip file with the binary

### Debian/Ubuntu
For Debian/Ubuntu we provide a .deb file. A one click installation package that does the job for you. \
Download the file here: \
Install from commandline by typing. \
`sudo apt install ./Hyperion-2.0.0-Linux-x86_64-x11.deb` \
Hyperion can be now started from your start menu.

### Fedora
For Fedora we provide a .rpm file. A one click installation package that does the job for you. \
Download the file here: \
Install from commandline by typing. \
`sudo dnf install ./Hyperion-2.0.0-Linux-x86_64-x11.rpm` \
Hyperion can be now started from your start menu.

## Uninstall Hyperion
On Debian/Ubuntu you can remove Hyperion with this command \
`sudo apt remove hyperion*` \

### Hyperion user data
Hyperion stores user data inside your home directory (folder `.hyperion`).