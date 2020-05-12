# Install Hyperion
Hyperion supports various platforms for installation, as package or portable .zip.

## Requirements

### Supported Systems
  * Raspberry Pi (See also [HyperBian](/en/user/HyperBian))
  * Debian 9 | Ubuntu 16.04 or higher
  * Mac OS

**Please note that some arm devices have limited support in terms of screen capturing**

### Supported Browsers
Hyperion will be configured and controlled trough a web interface.
  * Chrome 47+
  * Firefox 43+
  * Opera 34+
  * Safari 9.1+
  * Microsoft Edge 14+

::: warning Internet Explorer
Internet Explorer is not supported
:::

## Install Hyperion
  * Raspberry Pi you can use [HyperBian](/en/user/HyperBian.md) for a fresh start. Or use the install system
  * We provide installation packages (.deb) to install Hyperion with a single click on Debian/Ubuntu based systems.
  * Mac OSX - currently just a zip file with the binary

### Debian/Ubuntu
For Debian/Ubuntu we provide a .deb file. A one click installation package that does the job for you. \
Download the file from the [Release page](https://github.com/hyperion-project/hyperion.ng/releases) \
Install from commandline by typing. \
`sudo apt install ./Hyperion-2.0.0-Linux-x86_64.deb` \
Hyperion can be now started from your start menu.

### Fedora
For Fedora we provide a .rpm file. A one click installation package that does the job for you. \
Download the file from the [Release page](https://github.com/hyperion-project/hyperion.ng/releases) \
Install from commandline by typing. \
`sudo dnf install ./Hyperion-2.0.0-Linux-x86_64.rpm` \
Hyperion can be now started from your start menu.

## Uninstall Hyperion
On Debian/Ubuntu you can remove Hyperion with this command \
`sudo apt remove hyperion` \

### Hyperion user data
Hyperion stores user data inside your home directory (folder `.hyperion`).
