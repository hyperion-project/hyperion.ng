
# Installation
This page contains general installation steps for Hyperion.

## Windows & macOS
For Windows and macOS is an installation file available on our [Release page](https://github.com/hyperion-project/hyperion.ng/releases).

## Linux:
On the following operating systems, Hyperion can currently be installed/updated using the method listed below:
- Raspbian Buster/Raspberry Pi OS and later (armhf/arm64)
- Debian Buster(10) and later (armhf/arm64/x86_64)
- Ubuntu 20.04 and later (armhf/arm64/x86_64)

***

### Install Hyperion:
1. Add necessary packages for the installation:
```bash
sudo apt-get update && sudo apt-get install wget gpg apt-transport-https lsb-release
```

2. Add Hyperion’s official GPG key:
```bash
wget -qO- https://apt.hyperion-project.org/hyperion.pub.key | sudo gpg --dearmor -o /usr/share/keyrings/hyperion.pub.gpg
```

3. Add Hyperion-Project to your APT sources:
```bash
echo "deb [signed-by=/usr/share/keyrings/hyperion.pub.gpg] https://apt.hyperion-project.org/ $(lsb_release -cs) main" | sudo tee /etc/apt/sources.list.d/hyperion.list
```

4. Update your local package index and install Hyperion:
```bash
sudo apt-get update && sudo apt-get install hyperion
```
***

### Update Hyperion:
```bash
sudo apt-get install hyperion
```
***

### If you want to uninstall Hyperion, use the following commands:
1. Remove Hyperion:
```bash
sudo apt-get --purge autoremove hyperion
```

2. Remove the Hyperion-Project APT source from your system:
```bash
sudo rm /usr/share/keyrings/hyperion.pub.gpg /etc/apt/sources.list.d/hyperion.list
```
