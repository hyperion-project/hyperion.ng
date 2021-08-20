
# Installation

This page contains general installation steps for Hyperion. For Linux please follow the instructions below. For Windows is an installation file available on our [Release page](https://github.com/hyperion-project/hyperion.ng/releases).

## Linux:
On the following operating systems, Hyperion can currently be installed/updated using the method listed below:
- Raspbian Stretch/Raspberry Pi OS and later (armhf)
- Debian Stretch (9) and later (armhf/x64)
- Ubuntu 18.04 and later (x64)

***

### Install Hyperion:
1. Add Hyperion’s official GPG key:
```bash
wget -q0- https://apt.hyperion-project.org/hyperion.pub.key | sudo gpg --dearmor -o /usr/share/keyrings/hyperion.pub.gpg
```

2. Add Hyperion-Project to your APT sources:
```bash
echo "deb [signed-by=/usr/share/keyrings/hyperion.pub.gpg] https://apt.hyperion-project.org/ $(lsb_release -cs) main" | sudo tee /etc/apt/sources.list.d/hyperion.list
```

3. Update your local package index and install Hyperion:
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
sudo rm /usr/keyrings/hyperion.pub.gpg /etc/apt/sources.list.d/hyperion.list
```
